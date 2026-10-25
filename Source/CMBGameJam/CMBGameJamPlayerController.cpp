// Copyright Epic Games, Inc. All Rights Reserved.


#include "CMBGameJamPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "CMBGameJamCameraManager.h"
#include "CMBGameJamCharacter.h"
#include "Blueprint/UserWidget.h"
#include "CMBGameJam.h"
#include "Widgets/Input/SVirtualJoystick.h"

ACMBGameJamPlayerController::ACMBGameJamPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ACMBGameJamCameraManager::StaticClass();
}

void ACMBGameJamPlayerController::BeginPlay()
{
	Super::BeginPlay();


	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCMBGameJam, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Keyboard goes to the game, not to a focused widget
	SetInputMode(FInputModeGameOnly());

	// Command HUD
	if (CommandHUDClass)
	{
		CommandHUD = CreateWidget<UUserWidget>(this, CommandHUDClass);
		if (CommandHUD)
		{
			CommandHUD->AddToPlayerScreen(1);
		}
	}
}

void ACMBGameJamPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// Drone command bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			for (const TPair<TObjectPtr<UInputAction>, EDroneCommand>& Pair : CommandActions)
			{
				if (Pair.Key)
				{
					// Pair.Value is passed to OnCommandKey as the extra argument
					EnhancedInputComponent->BindAction(Pair.Key, ETriggerEvent::Started, this,
						&ACMBGameJamPlayerController::OnCommandKey, Pair.Value);
				}
			}

			if (ConfirmAction)
			{
				EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ACMBGameJamPlayerController::OnConfirm);
			}

			if (EraseAction)
			{
				EnhancedInputComponent->BindAction(EraseAction, ETriggerEvent::Started, this, &ACMBGameJamPlayerController::OnErase);
			}

			if (StopAction)
			{
				EnhancedInputComponent->BindAction(StopAction, ETriggerEvent::Started, this, &ACMBGameJamPlayerController::OnStop);
			}
		}
		else
		{
			UE_LOG(LogCMBGameJam, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
		}
	}

}

void ACMBGameJamPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	Drone = Cast<ACMBGameJamCharacter>(InPawn);
	if (Drone)
	{
		Drone->OnFinished.AddUniqueDynamic(this, &ACMBGameJamPlayerController::OnDroneFinished);
		Drone->OnCommandGlitch.AddUniqueDynamic(this, &ACMBGameJamPlayerController::OnDroneGlitch);
	}
	else
	{
		UE_LOG(LogCMBGameJam, Warning, TEXT("Possessed pawn '%s' is not an ACMBGameJamCharacter."), *GetNameSafe(InPawn));
	}
}

void ACMBGameJamPlayerController::OnUnPossess()
{
	if (Drone)
	{
		Drone->OnFinished.RemoveDynamic(this, &ACMBGameJamPlayerController::OnDroneFinished);
		Drone->OnCommandGlitch.RemoveDynamic(this, &ACMBGameJamPlayerController::OnDroneGlitch);
		Drone = nullptr;
	}

	Super::OnUnPossess();
}

bool ACMBGameJamPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

FText ACMBGameJamPlayerController::GetCommandLabel(EDroneCommand Command)
{
	return StaticEnum<EDroneCommand>()->GetDisplayNameTextByValue(static_cast<int64>(Command));
}

void ACMBGameJamPlayerController::OnCommandKey(const FInputActionInstance& Instance, EDroneCommand Command)
{
	// Once the queue is full, letters can no longer be committed, so ignore them
	if (CommandQueue.Num() >= MaxCommands)
	{
		return;
	}

	PendingCommand = Command;
	OnBufferChanged.Broadcast();
}

void ACMBGameJamPlayerController::OnConfirm()
{
	const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

	if (bShift)
	{
		SendQueue();
	}
	else
	{
		CommitPending();
	}
}

void ACMBGameJamPlayerController::OnStop()
{
	if (Drone)
	{
		// Takes effect after the drone's command delay, like any other command
		Drone->RequestStop();
	}
}

void ACMBGameJamPlayerController::OnErase()
{
	if (PendingCommand != EDroneCommand::None)
	{
		PendingCommand = EDroneCommand::None;
	}
	else if (CommandQueue.Num() > 0)
	{
		CommandQueue.Pop();
	}

	OnBufferChanged.Broadcast();
}

void ACMBGameJamPlayerController::CommitPending()
{
	if (PendingCommand == EDroneCommand::None || CommandQueue.Num() >= MaxCommands)
	{
		return;
	}

	CommandQueue.Add(PendingCommand);
	PendingCommand = EDroneCommand::None;
	OnBufferChanged.Broadcast();
}

void ACMBGameJamPlayerController::SendQueue()
{
	// Shift+Enter also takes the letter currently on screen
	CommitPending();

	if (CommandQueue.IsEmpty() || !Drone)
	{
		return;
	}

	// The drone lines these up behind everything sent before, spaced by its command delay
	Drone->SubmitCommands(CommandQueue);
	CommandQueue.Reset();
	OnBufferChanged.Broadcast();
}

void ACMBGameJamPlayerController::OnDroneFinished()
{
	OnBufferChanged.Broadcast();
}

void ACMBGameJamPlayerController::OnDroneGlitch(EDroneCommand Intended, EDroneCommand Actual)
{
	OnPacketLost.Broadcast(Intended, Actual);
}
