// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DroneCommand.h"
#include "CMBGameJamPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class ACMBGameJamCharacter;
struct FInputActionInstance;

/** Fired whenever the pending letter or the queue changes, or the drone finishes. The HUD listens to this. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCommandBufferChanged);

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 *  Holds the drone command buffer.
 */
UCLASS(abstract, config="Game")
class CMBGAMEJAM_API ACMBGameJamPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	ACMBGameJamPlayerController();

	/** Maximum number of queued commands */
	static constexpr int32 MaxCommands = 5;

	/** The letter currently on screen (flashing). Not queued until Enter. */
	UPROPERTY(BlueprintReadOnly, Category="Drone")
	EDroneCommand PendingCommand = EDroneCommand::None;

	/** Committed commands, waiting for Shift+Enter */
	UPROPERTY(BlueprintReadOnly, Category="Drone")
	TArray<EDroneCommand> CommandQueue;

	UPROPERTY(BlueprintAssignable, Category="Drone")
	FOnCommandBufferChanged OnBufferChanged;

	/** Display name for a command ("Turn Right"), for the HUD */
	UFUNCTION(BlueprintPure, Category="Drone")
	static FText GetCommandLabel(EDroneCommand Command);

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Input Action -> drone command. One row per command. */
	UPROPERTY(EditAnywhere, Category="Drone|Input")
	TMap<TObjectPtr<UInputAction>, EDroneCommand> CommandActions;

	/** Enter commits the letter; Shift+Enter sends the queue */
	UPROPERTY(EditAnywhere, Category="Drone|Input")
	TObjectPtr<UInputAction> ConfirmAction;

	/** Backspace removes the pending letter, then the last committed command */
	UPROPERTY(EditAnywhere, Category="Drone|Input")
	TObjectPtr<UInputAction> EraseAction;

	/** Halts the drone immediately. Not queued, and it leaves the typed commands alone. */
	UPROPERTY(EditAnywhere, Category="Drone|Input")
	TObjectPtr<UInputAction> StopAction;

	/** HUD showing the flashing letter and the 5 slots */
	UPROPERTY(EditAnywhere, Category="Drone|UI")
	TSubclassOf<UUserWidget> CommandHUDClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> CommandHUD;

	/** The drone being controlled: the possessed character */
	UPROPERTY()
	TObjectPtr<ACMBGameJamCharacter> Drone;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Hooks up the drone's OnFinished when we possess it */
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	/** A command key was pressed. Replaces the pending letter. */
	void OnCommandKey(const FInputActionInstance& Instance, EDroneCommand Command);

	/** Enter / Shift+Enter */
	void OnConfirm();

	/** Backspace */
	void OnErase();

	/** Stop key */
	void OnStop();

	/** Moves the pending letter into the queue */
	void CommitPending();

	/** Commits the pending letter, then hands the queue to the drone */
	void SendQueue();

	UFUNCTION()
	void OnDroneFinished();
};
