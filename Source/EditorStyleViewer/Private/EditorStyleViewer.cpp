// Copyright (C) Vasily Bulgakov. 2024. All Rights Reserved.

#include "EditorStyleViewer.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "HAL/PlatformApplicationMisc.h"

static const FName EditorStyleViewerTabName("EditorStyleViewer");

#define LOCTEXT_NAMESPACE "FEditorStyleViewerModule"




struct FEditorStylePreviewOption
{
	FText Title;
	FText Tooltip;

	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FCreateWidget, FName /*Style*/);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FCanUseStyle, FName /*Style*/);
	
	FCreateWidget CreateWidget;
	FCanUseStyle CanUseStyle;

	FEditorStylePreviewOption(FText Title, FText Tooltip, FCreateWidget CreateWidget, FCanUseStyle CanUseStyle = FCanUseStyle())
		: Title(Title)
		, Tooltip(Tooltip)
		, CreateWidget(CreateWidget)
		, CanUseStyle(CanUseStyle)
	{
		
	}
};	


class SEditorStyleViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEditorStyleViewer)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TArray<FEditorStylePreviewOption> InPreviewOptions)
	{
		for (int32 Index = 0; Index < InPreviewOptions.Num(); Index++)
		{
			PreviewOptions.Add(*FString::Printf(TEXT("%d_%s"), Index, *InPreviewOptions[Index].Title.ToString()), InPreviewOptions[Index]);
		}		
		
		ChildSlot
		[			
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock).Text(this, &SEditorStyleViewer::GetNumOptions)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(SearchField, SEditableTextBox)
					.HintText(LOCTEXT("Search", "Search"))
					.OnTextChanged(this, &SEditorStyleViewer::OnSearchTextChanged)
					.OnTextCommitted(this, &SEditorStyleViewer::OnSearchTextCommitted)
				]
				+ SVerticalBox::Slot()
				[
					SAssignNew(OptionListView, SListView<TSharedPtr<FString>>)
					.ItemHeight(20.0f)
					.ListItemsSource(&FilteredOptionList)
					.OnGenerateRow(this, &SEditorStyleViewer::OnGenerateRow)
					.OnSelectionChanged(this, &SEditorStyleViewer::OnSelectionChanged)
					.SelectionMode(ESelectionMode::Single)		
				]				
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.AutoHeight()
				[
					CreateToolbar()
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
						.Text(LOCTEXT("Copy", "Copy Style Name"))
						.OnClicked(this, &SEditorStyleViewer::CopyCurrentSyle)
					]
					+ SHorizontalBox::Slot().VAlign(VAlign_Center).AutoWidth()
					[
						SNew(STextBlock).Text(this, &SEditorStyleViewer::GetCurrentStyle)
					]
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SAssignNew(ContentPanel, SBox)
				]
			]
		];

		
		for(auto& Option : PreviewOptions)
		{
			SetPreview(Option.Key);
			break;
		}
	}

	TSharedRef<SWidget> CreateToolbar()
	{
		TSharedRef<SWrapBox> Panel = SNew(SWrapBox).UseAllottedSize(true);
		for (auto& Pair : PreviewOptions)
		{
			if (!Pair.Value.CreateWidget.IsBound())
			{
				Panel->AddSlot()
				[
					SNew(STextBlock).Text(LOCTEXT("Error_NotBound", "Not bound"))
				];
			}
			Panel->AddSlot()
			.Padding(4, 2)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("FilledBorder"))
				.BorderBackgroundColor(this, &SEditorStyleViewer::GetButtonHightlight, Pair.Key)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.Text(Pair.Value.Title)
					.ToolTipText(Pair.Value.Tooltip.IsEmpty() ? Pair.Value.Title : Pair.Value.Tooltip)
					.OnClicked(this, &SEditorStyleViewer::OnPreviewWidgetSelected, Pair.Key)
				]
			];
		}		
		return Panel;
	}

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*InItem))
			.ToolTipText(FText::FromString(*InItem))
		];
	}
	void OnSelectionChanged(TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo)
	{		
		SetStyle(Selection ? *Selection : TEXT(""));
	}

	FReply OnPreviewWidgetSelected(FName Name)
	{
		SetPreview(Name);
		return FReply::Handled();
	}

	FText GetCurrentStyle() const
	{
		return FText::FromString(FString::Printf(TEXT("%s: %s"), *CurrentPreview.ToString(), *SelectedStyle.ToString()));
	}

	FText GetNumOptions() const
	{
		return FText::FromString(FString::Printf(TEXT("Available: %d / %d"), FilteredOptionList.Num(), OptionList.Num()));
	}

	FReply CopyCurrentSyle()
	{
		FPlatformApplicationMisc::ClipboardCopy(*SelectedStyle.ToString());
		return FReply::Handled();
	}

	FSlateColor GetButtonHightlight(FName Name) const
	{
		if (CurrentPreview == Name)
		{
			return FLinearColor::Gray;
		}
		return FLinearColor::Transparent;
	}

	void SetStyle(FString Style)
	{
		SelectedStyle = FName(*Style);
		RefreshPreview();
	}
	
	void SetPreview(FName Type)
	{
		CurrentPreview = Type;
		
		if (FEditorStylePreviewOption* Option = PreviewOptions.Find(CurrentPreview))
		{
			OptionList.Reset();

			bool bFilterStyle = Option->CanUseStyle.IsBound();
			TSet<FName> Names = FAppStyle::Get().GetStyleKeys();	
			for (const FName& Name : Names)
			{
				if (bFilterStyle && !Option->CanUseStyle.Execute(Name))
				{
					continue;
				}
				OptionList.Add(MakeShared<FString>(Name.ToString()));
			}

			if (bFilterStyle && !Option->CanUseStyle.Execute(SelectedStyle))
			{
				SelectedStyle = NAME_None;
			}
		}
		
		RefreshOptions();
		RefreshPreview();
	}

	void RefreshPreview()
	{
		TSharedPtr<SWidget> Preview;		
		
		if (FEditorStylePreviewOption* Option = PreviewOptions.Find(CurrentPreview))
		{
			if (Option->CreateWidget.IsBound())
			{
				Preview = Option->CreateWidget.Execute(SelectedStyle);
			}		
		}

		if (Preview.IsValid())
		{
			ContentPanel->SetContent(Preview.ToSharedRef());
		}
		else
		{
			ContentPanel->SetContent(SNew(STextBlock).Text(LOCTEXT("Error_NoWidget", "Not widget")));
		}		
	}	

	void OnSearchTextChanged(const FText& ChangedText)
	{
		SearchText = ChangedText;

		RefreshOptions();
	}

	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
	{
		if ((InCommitType == ETextCommit::Type::OnEnter) && FilteredOptionList.Num() > 0)
		{
			OptionListView->SetSelection(FilteredOptionList[0], ESelectInfo::OnKeyPress);
		}
	}

	void RefreshOptions()
	{
		// Need to refresh filtered list whenever options change
		FilteredOptionList.Reset();

		if (SearchText.IsEmpty())
		{
			FilteredOptionList.Append(OptionList);
		}
		else
		{
			for (TSharedPtr<FString> Option : OptionList)
			{
				if (SearchText.IsEmpty() || Option->Find(SearchText.ToString(), ESearchCase::Type::IgnoreCase) >= 0)
				{
					FilteredOptionList.Add(Option);
				}
			}
		}

		if (!SelectedStyle.IsNone())
		{
			for (TSharedPtr<FString> Option : FilteredOptionList)
			{
				if (FName(*Option) == SelectedStyle)
				{
					OptionListView->SetSelection(Option);
					break;
				}
			}
		}

		OptionListView->RequestListRefresh();
	}

private:
	FName CurrentPreview;
	FName SelectedStyle;
	
	TMap<FName, FEditorStylePreviewOption> PreviewOptions;
	
	TSharedPtr<SBox> ContentPanel;	
	
	TSharedPtr<SEditableTextBox> SearchField;
	FText SearchText;
	
	TArray<TSharedPtr<FString>> OptionList;
	TArray<TSharedPtr<FString>> FilteredOptionList;
	TSharedPtr<SListView<TSharedPtr<FString>>> OptionListView;
};







void FEditorStyleViewerModule::StartupModule()
{		
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		EditorStyleViewerTabName,
		FOnSpawnTab::CreateRaw(this, &FEditorStyleViewerModule::OnSpawnPluginTab)
	)
	.SetDisplayName(LOCTEXT("FEditorStyleViewerTabTitle", "Editor Style Viewer"))
	.SetTooltipText(LOCTEXT("FEditorStyleViewerTabTooltip", "Browse editor styles"))
	.SetIcon(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "ClassThumbnail.SlateWidgetStyleAsset"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());
}

void FEditorStyleViewerModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EditorStyleViewerTabName);
	}
}

TSharedRef<SDockTab> FEditorStyleViewerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	typedef FEditorStylePreviewOption::FCreateWidget FWidgetDelegate;
	typedef FEditorStylePreviewOption::FCanUseStyle FFilterDelegate;
	
	TArray<FEditorStylePreviewOption> PreviewOptions;
	
	PreviewOptions.Add(FEditorStylePreviewOption(
		LOCTEXT("Image", "Image"),
		FText::GetEmpty(),
		FWidgetDelegate::CreateLambda([](FName StyleName)
		{
			return SNew(SImage).Image(FAppStyle::GetBrush(StyleName));
		}),
		FFilterDelegate::CreateLambda([](FName StyleName)
		{
			return FAppStyle::Get().GetOptionalBrush(StyleName) != FAppStyle::GetNoBrush();
		})
	));

	PreviewOptions.Add(FEditorStylePreviewOption(
		LOCTEXT("Image500", "Image 500"),
		FText::GetEmpty(),
		FWidgetDelegate::CreateLambda([](FName StyleName)
		{
			return
				SNew(SBox).WidthOverride(500).HeightOverride(500)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					[
						SNew(SImage).Image(FAppStyle::GetBrush(StyleName))
					]
				];
		}),
		FFilterDelegate::CreateLambda([](FName StyleName)
		{
			return FAppStyle::Get().GetOptionalBrush(StyleName) != FAppStyle::GetNoBrush();
		})
	));

	PreviewOptions.Add(FEditorStylePreviewOption(
    	LOCTEXT("Color", "Color"),
    	FText::GetEmpty(),
    	FWidgetDelegate::CreateLambda([](FName StyleName)
    	{
    		return SNew(SBox).WidthOverride(100).HeightOverride(100)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("FilledBorder"))
				.ColorAndOpacity(FAppStyle::GetColor(StyleName))
			];
    	})
    ));	

	PreviewOptions.Add(FEditorStylePreviewOption(
		LOCTEXT("Text", "Text"),
		FText::GetEmpty(),
		FWidgetDelegate::CreateLambda([](FName StyleName)
		{
			TSharedRef<SEditableTextBox> EditableText = SNew(SEditableTextBox);
			EditableText->SetText(LOCTEXT("TestTest", "The quick brown fox jumps over the lazy dog"));
			
			return SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(5).HAlign(HAlign_Center)
				[
					EditableText
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), StyleName)
					.Text_Lambda([WeakSource = TWeakPtr<SEditableTextBox>(EditableText)]()
					{
						if (WeakSource.IsValid())
						{
							return WeakSource.Pin()->GetText();
						}
						return FText::GetEmpty();
					})
				];			
		}),
		FFilterDelegate::CreateLambda([](FName StyleName)
		{
			return FAppStyle::Get().HasWidgetStyle<FTextBlockStyle>(StyleName);
		})
	));

	PreviewOptions.Add(FEditorStylePreviewOption(
		LOCTEXT("Button", "Button"),
		FText::GetEmpty(),
		FWidgetDelegate::CreateLambda([](FName StyleName)
		{			
			return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), StyleName)
			.Text(LOCTEXT("ButtonText", "Button"));				
		}),
		FFilterDelegate::CreateLambda([](FName StyleName)
		{
			return FAppStyle::Get().HasWidgetStyle<FButtonStyle>(StyleName);
		})
	));

	
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SEditorStyleViewer, PreviewOptions)
		];
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEditorStyleViewerModule, EditorStyleViewer)