// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DroneCommand.generated.h"

/** Everything the player can queue. Add new commands here, then add a row in ACMBGameJamCharacter::Steps. */
UENUM(BlueprintType)
enum class EDroneCommand : uint8
{
	None		UMETA(Hidden),
	Forward		UMETA(DisplayName = "Forward"),
	Back		UMETA(DisplayName = "Backward"),
	Left		UMETA(DisplayName = "Left"),
	Right		UMETA(DisplayName = "Right"),
	Jump		UMETA(DisplayName = "Jump"),
	TurnRight	UMETA(DisplayName = "Turn Right"),
	TurnLeft	UMETA(DisplayName = "Turn Left"),
};

/**
 * What a command does once it starts. Commands stack: Forward then Right moves diagonally.
 * A command replaces any active one that drives the same axis (Forward replaces Backward, TurnLeft replaces TurnRight).
 * Nothing else ends it: a movement or turn keeps going until its counterpart or Stop.
 * Jump is not listed here, it is a one-off impulse.
 * Edit these on BP_DronePlayer.
 */
USTRUCT(BlueprintType)
struct FDroneStep
{
	GENERATED_BODY()

	/** Direction to move, relative to where the character faces. Only the direction counts (Z ignored); speed comes from Max Walk Speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LocalMove = FVector::ZeroVector;

	/** Turn speed in degrees per second (positive = right) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float YawRate = 0.f;
};
