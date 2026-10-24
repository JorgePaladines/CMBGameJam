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

/** What one command does. Edit these on BP_DronePlayer. */
USTRUCT(BlueprintType)
struct FDroneStep
{
	GENERATED_BODY()

	/** Horizontal movement relative to the character's current facing (cm). Z is ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LocalMove = FVector::ZeroVector;

	/** Yaw change in degrees (positive = right) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float YawDelta = 0.f;

	/** Jump, then wait until landing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bJump = false;
};