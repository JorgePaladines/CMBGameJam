// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "DroneCommand.h"
#include "CMBGameJamCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UAudioComponent;
class USoundBase;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Fired when the last queued command finishes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDroneFinished);

/** Fired when DifficultyScale changes. For UI. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDifficultyChanged, float, NewScale);

/** Fired when a technical difficulty hit a command: Actual is None if it was skipped, or another command if it was swapped */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCommandGlitch, EDroneCommand, Intended, EDroneCommand, Actual);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ACMBGameJamCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;
	
	/** A command waiting for its turn to start */
	struct FPendingCommand
	{
		EDroneCommand Command;
		double ExecuteAt;
	};

	/** A command that is currently driving the drone */
	struct FActiveAction
	{
		FDroneStep Step;
	};

	/** Commands sent but not started yet, in order. Every list sent lines up behind the previous ones. */
	TArray<FPendingCommand> Pending;

	/** Movement/turn commands running right now. They stack. */
	TArray<FActiveAction> Active;

	/** When the last pending command is due; the next command goes after this plus the delay */
	double LastScheduledTime = 0.0;

	/** When a requested Stop takes effect, or < 0 if none */
	double StopAt = -1.0;

	/** When to release the jump input after a Jump, or < 0 if none */
	double JumpReleaseAt = -1.0;

	/** Which of PitchLevels the camera is heading to. INDEX_NONE until the first look command. */
	int32 PitchLevelIndex = INDEX_NONE;

	/** True while the camera is still moving toward the target pitch */
	bool bPitching = false;

	/** Whether each loop is currently on, so fades are only started on a change */
	bool bMoveSoundOn = false;
	bool bTurnSoundOn = false;

public:
	ACMBGameJamCharacter();

	/**
	 * Adds the commands to the end of the pipeline. Each one starts GetEffectiveDelay() seconds after the previous one,
	 * counting every list sent before it, and keeps running (stacking with the others) until Stop or its counterpart.
	 */
	UFUNCTION(BlueprintCallable, Category="Drone")
	void SubmitCommands(const TArray<EDroneCommand>& Commands);

	/**
	 * Asks the drone to stop. Takes effect GetEffectiveDelay() seconds from now, then cancels everything running
	 * and everything still waiting. It does not queue behind pending commands.
	 */
	UFUNCTION(BlueprintCallable, Category="Drone")
	void RequestStop();

	/** Halts the drone right now and drops everything, ignoring the delay. */
	UFUNCTION(BlueprintCallable, Category="Drone")
	void StopDrone();

	/** True while anything is pending, running, or a stop is on its way */
	UFUNCTION(BlueprintPure, Category="Drone")
	bool IsExecuting() const { return !Pending.IsEmpty() || !Active.IsEmpty() || StopAt >= 0.0; }

	/**
	 * Base seconds between one command and the next, and before a Stop takes effect.
	 * DifficultyScale adds its own delay on top. Applies to commands sent afterwards.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone", meta = (ClampMin = "0.0", Units = "s"))
	float CommandDelay = 0.f;

	/** Fired when the drone has nothing left to do */
	UPROPERTY(BlueprintAssignable, Category="Drone")
	FOnDroneFinished OnFinished;

	/** Technical difficulties, 0 to 1 (a percentage). Change it with SetDifficultyScale; read it here or with GetDifficultyScale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Drone|Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DifficultyScale = 0.f;

	/** Sets DifficultyScale (clamped to 0..1) and notifies listeners. Callable from any Blueprint. */
	UFUNCTION(BlueprintCallable, Category="Drone|Difficulty")
	void SetDifficultyScale(float NewScale);

	UFUNCTION(BlueprintPure, Category="Drone|Difficulty")
	float GetDifficultyScale() const { return DifficultyScale; }

	/** The delay actually used between commands right now: CommandDelay plus the difficulty delay */
	UFUNCTION(BlueprintPure, Category="Drone|Difficulty")
	float GetEffectiveDelay() const;

	/** Chance a command is skipped, 0..1, at the current scale */
	UFUNCTION(BlueprintPure, Category="Drone|Difficulty")
	float GetSkipChance() const { return DifficultyScale * MaxSkipChance; }

	/** Chance a command is swapped for a random one, 0..1, at the current scale */
	UFUNCTION(BlueprintPure, Category="Drone|Difficulty")
	float GetRandomizeChance() const { return DifficultyScale * MaxRandomizeChance; }

	/** Fired when DifficultyScale changes */
	UPROPERTY(BlueprintAssignable, Category="Drone|Difficulty")
	FOnDifficultyChanged OnDifficultyChanged;

	/** Fired when a command is skipped or swapped, so the UI can react */
	UPROPERTY(BlueprintAssignable, Category="Drone|Difficulty")
	FOnCommandGlitch OnCommandGlitch;

protected:

	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Command -> move direction and turn direction. Tune them on BP_DronePlayer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone")
	TMap<EDroneCommand, FDroneStep> Steps;

	/** How fast the drone turns while a turn command is active, in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone", meta = (ClampMin = "0.0", Units = "deg/s"))
	float TurnSpeed = 60.f;

	/**
	 * Pitch angles the camera can look at, lowest to highest. Look Up moves to the next one, Look Down to the previous,
	 * and both stop at the ends. Positive looks up.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Look")
	TArray<float> PitchLevels = { -30.f, 0.f, 30.f };

	/** How fast the camera moves between pitch levels, in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Look", meta = (ClampMin = "0.0", Units = "deg/s"))
	float PitchSpeed = 90.f;

	/** Extra seconds of delay at DifficultyScale 1 (scales linearly from 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Difficulty", meta = (ClampMin = "0.0", Units = "s"))
	float MaxDifficultyDelay = 1.f;

	/** Chance a command is skipped at DifficultyScale 1 (scales linearly from 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxSkipChance = 0.8f;

	/** Chance a command is swapped for a random one at DifficultyScale 1 (scales linearly from 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxRandomizeChance = 0.5f;

	/** Rolls the technical difficulties for one command: returns it unchanged, another command, or None if skipped */
	EDroneCommand ApplyGlitch(EDroneCommand Intended) const;

	/** Plays while any movement command is active. Needs a sound set to loop (Looping on the Sound Wave, or a looping Sound Cue). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Audio")
	TObjectPtr<USoundBase> MoveLoopSound;

	/** Plays while turning or while the camera is looking up/down. Needs a sound set to loop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Audio")
	TObjectPtr<USoundBase> TurnLoopSound;

	/** Played once each time the drone jumps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Audio")
	TObjectPtr<USoundBase> JumpSound;

	/** Seconds to fade the loops in and out. 0 = start and stop instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone|Audio", meta = (ClampMin = "0.0", Units = "s"))
	float LoopFadeTime = 0.15f;

	/** The components that play the loops. Their attenuation can be tuned on BP_DronePlayer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|Audio")
	TObjectPtr<UAudioComponent> MoveAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Drone|Audio")
	TObjectPtr<UAudioComponent> TurnAudio;

	/** Starts or stops the loops to match what the drone is doing right now */
	void RefreshLoopSounds();

	/** Starts every pending command that is due */
	void ProcessPending(double Now);

	/** Starts one command: a jump, or a movement/turn that stacks with the active ones */
	void Activate(EDroneCommand Command, double Now);

	/** Ticks only while there is work, and fires OnFinished when the work runs out */
	void UpdateTickState();

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
};

