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
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Fired when the last queued command finishes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDroneFinished);

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

public:
	ACMBGameJamCharacter();

	/**
	 * Adds the commands to the end of the pipeline. Each one starts CommandDelay seconds after the previous one,
	 * counting every list sent before it, and keeps running (stacking with the others) until Stop or its counterpart.
	 */
	UFUNCTION(BlueprintCallable, Category="Drone")
	void SubmitCommands(const TArray<EDroneCommand>& Commands);

	/**
	 * Asks the drone to stop. Takes effect CommandDelay seconds from now, then cancels everything running
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
	 * Seconds between one command and the next, and before a Stop takes effect.
	 * Change it at any time (also from Blueprint); it applies to commands sent afterwards.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drone", meta = (ClampMin = "0.0", Units = "s"))
	float CommandDelay = 0.f;

	/** Fired when the drone has nothing left to do */
	UPROPERTY(BlueprintAssignable, Category="Drone")
	FOnDroneFinished OnFinished;

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

