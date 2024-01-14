// Georgy Treshchev 2024.

#pragma once

#include "SlateExtras.h"
#include "Misc/EngineVersionComparison.h"
#if UE_VERSION_OLDER_THAN(5, 0, 0)
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#else
#include "Styling/StarshipCoreStyle.h"
#endif
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"

/**
 * Delegate fired when the cancel button is clicked in the speech recognizer progress window
 */
DECLARE_DELEGATE(FOnSpeechRecognizerCancelClicked);

/**
 * Delegate to retrieve the percentage of progress to display in the speech recognizer progress window
 * @return The percentage of progress, as a float between 0 and 1
 */
DECLARE_DELEGATE_RetVal(float, FOnSpeechRecognizerGetPercentage);

/**
 * Widget designed to block the UI and display a progress bar and text while a long-running task is being performed
 * Might potentially be replaced with FSlowTask, although FSlowTask is not supported in the project settings
 */
class SSpeechRecognizerProgressWindow : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SSpeechRecognizerProgressWindow) {}
		SLATE_ARGUMENT(FText, Title)
		SLATE_EVENT(FOnSpeechRecognizerCancelClicked, OnCancelClicked)
		SLATE_EVENT(FOnSpeechRecognizerGetPercentage, OnGetPercentage)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Title = InArgs._Title;
		OnCancelClicked = InArgs._OnCancelClicked;
		OnGetPercentage = InArgs._OnGetPercentage;

		TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .Padding(FMargin(0, 0, 0, 5.f))
			  .VAlign(VAlign_Center)
			[
				SNew(SBox)
				.HeightOverride(24.f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					[
						SNew(STextBlock)
							.AutoWrapText(true)
							.Text(Title)
#if UE_VERSION_OLDER_THAN(5, 0, 0)
							.Font(FCoreStyle::GetDefaultFontStyle("Light", 14))
#else
							.Font(FStarshipCoreStyle::GetDefaultFontStyle("NormalFont", 14))
#endif
					]

					+ SHorizontalBox::Slot()
					  .Padding(FMargin(5.f, 0, 0, 0))
					  .AutoWidth()
					[
						SNew(STextBlock)
							.Text(this, &SSpeechRecognizerProgressWindow::GetPercentageText)
#if UE_VERSION_OLDER_THAN(5, 0, 0)
							.Font(FCoreStyle::GetDefaultFontStyle("Light", 14))
#else
							.Font(FStarshipCoreStyle::GetDefaultFontStyle("NormalFont", 14))
#endif
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(12.0f)
				[
					SNew(SProgressBar)
						.BorderPadding(FVector2D::ZeroVector)
						.Percent(this, &SSpeechRecognizerProgressWindow::GetPercentageValue)
				]
			]

			+ SVerticalBox::Slot()
			  .AutoHeight()
			  .HAlign(HAlign_Center)
			  .Padding(10.0f, 7.0f)
			[
				SNew(SButton)
					.Text(NSLOCTEXT("FeedbackContextProgress", "Cancel", "Cancel"))
					.HAlign(EHorizontalAlignment::HAlign_Center)
					.OnClicked(this, &SSpeechRecognizerProgressWindow::HandleCancelButtonClicked)
			]
		];

		SBorder::Construct(SBorder::FArguments()
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		                   .BorderImage(FEditorStyle::GetBrush("Menu.Background"))
#else
		                   .BorderImage(FAppStyle::GetBrush("Brushes.Header"))
#endif
		                   .VAlign(VAlign_Center)
		                   .Padding(FMargin(24))
			[
				SNew(SBox).
				WidthOverride(600)
				[
					VerticalBox
				]
			]
		);
	}

private:
	/** The title of the progress window */
	TAttribute<FText> Title;

	/** Delegate fired when the cancel button is clicked */
	FOnSpeechRecognizerCancelClicked OnCancelClicked;

	/** Delegate to retrieve the percentage of progress */
	FOnSpeechRecognizerGetPercentage OnGetPercentage;

	/** Pointer to the progress bar widget in the progress window */
	TSharedPtr<SProgressBar> ProgressBar;

	/** Returns the percentage of progress to display in the progress window */
	FText GetPercentageText() const
	{
		return FText::Format(NSLOCTEXT("SpeechRecognizerProgress", "Progress", "{0}%"), FText::AsNumber(GetPercentageValue().Get(0) * 100.f));
	}

	/** Returns the value of progress as a float between 0 and 1 */
	TOptional<float> GetPercentageValue() const
	{
		if (OnGetPercentage.IsBound())
		{
			return OnGetPercentage.Execute();
		}
		return 0.0f;
	}

	/** Handles when the cancel button is clicked */
	FReply HandleCancelButtonClicked()
	{
		if (OnCancelClicked.IsBound())
		{
			OnCancelClicked.Execute();
		}
		return FReply::Handled();
	}
};

/**
 * Wrapper class to display a speech recognizer progress window as a modal window
 */
class FSpeechRecognizerProgressDialog
{
public:
	/**
	 * Constructs the speech recognizer progress window with the given parameters and displays it as a modal window
	 * @param Title The title of the progress window
	 * @param Message The message to display in the progress window
	 * @param OnGetPercentage Delegate to retrieve the percentage of progress
	 * @param OnCancelClicked Delegate fired when the cancel button is clicked
	 */
	FSpeechRecognizerProgressDialog(const FText& Title, const FText& Message, const FOnSpeechRecognizerGetPercentage& OnGetPercentage, const FOnSpeechRecognizerCancelClicked& OnCancelClicked)
	{
		ProgressWindow = SNew(SSpeechRecognizerProgressWindow)
		.Title(Message)
		.OnGetPercentage(OnGetPercentage)
		.OnCancelClicked(OnCancelClicked);

		MainWindow = SNew(SWindow)
		.Title(Title)
		.SizingRule(ESizingRule::Autosized)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.IsPopupWindow(true)
		.CreateTitleBar(true)
		.ActivationPolicy(EWindowActivationPolicy::Always)
		.FocusWhenFirstShown(true);

		MainWindow->SetContent(ProgressWindow.ToSharedRef());

		FSlateApplication::Get().AddModalWindow(MainWindow.ToSharedRef(), nullptr, true);
		MainWindow->ShowWindow();
	}

	~FSpeechRecognizerProgressDialog()
	{
		MainWindow->RequestDestroyWindow();
		MainWindow.Reset();
	}

	/** Pointer to the SSpeechRecognizerProgressWindow widget */
	TSharedPtr<SSpeechRecognizerProgressWindow> ProgressWindow;

	/** Pointer to the SWindow object that contains the progress window */
	TSharedPtr<SWindow> MainWindow;
};
