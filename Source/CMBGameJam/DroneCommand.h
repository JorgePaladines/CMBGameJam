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
	LookUp		UMETA(DisplayName = "Look Up"),
	LookDown	UMETA(DisplayName = "Look Down"),
};

/**
 * What a command does once it starts. Commands stack: Forward then Right moves diagonally.
 * A command replaces any active one that drives the same axis (Forward replaces Backward, TurnLeft replaces TurnRight).
 * A turn keeps going until its counterpart, Stop, or the next movement command (which cancels it).
 * A movement keeps going until its counterpart or Stop, and turns stack on top of it.
 * Jump, LookUp and LookDown are not listed here: Jump is a one-off impulse, and the look commands step the camera
 * between the character's Pitch Levels.
 * Edit these on BP_DronePlayer.
 */
USTRUCT(BlueprintType)
struct FDroneStep
{
	GENERATED_BODY()

	/** Direction to move, relative to where the character faces. Only the direction counts (Z ignored); speed comes from Max Walk Speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LocalMove = FVector::ZeroVector;

	/** Which way to turn: 1 = right, -1 = left, 0 = no turn. How fast is the character's Turn Speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Turn = 0.f;
};
