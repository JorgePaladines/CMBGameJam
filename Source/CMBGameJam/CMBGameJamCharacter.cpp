// Copyright Epic Games, Inc. All Rights Reserved.

#include "CMBGameJamCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "CMBGameJam.h"

ACMBGameJamCharacter::ACMBGameJamCharacter()
{
	// Only ticks while a command list is running
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Default steps so it works before you touch the Blueprint. Tune them there afterwards.
	auto MakeStep = [](const FVector& Move, float Turn)
	{
		FDroneStep Step;
		Step.LocalMove = Move;
		Step.Turn = Turn;
		return Step;
	};

	// Jump has no row: it is a one-off impulse handled in Activate
	Steps.Add(EDroneCommand::Forward,   MakeStep(FVector( 1.f,  0.f, 0.f),  0.f));
	Steps.Add(EDroneCommand::Back,      MakeStep(FVector(-1.f,  0.f, 0.f),  0.f));
	Steps.Add(EDroneCommand::Left,      MakeStep(FVector( 0.f, -1.f, 0.f),  0.f));
	Steps.Add(EDroneCommand::Right,     MakeStep(FVector( 0.f,  1.f, 0.f),  0.f));
	Steps.Add(EDroneCommand::TurnRight, MakeStep(FVector::ZeroVector,       1.f));
	Steps.Add(EDroneCommand::TurnLeft,  MakeStep(FVector::ZeroVector,      -1.f));
}

void ACMBGameJamCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		// EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACMBGameJamCharacter::DoJumpStart);
		// EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACMBGameJamCharacter::DoJumpEnd);
	}
	else
	{
		UE_LOG(LogCMBGameJam, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ACMBGameJamCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ACMBGameJamCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ACMBGameJamCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ACMBGameJamCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ACMBGameJamCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ACMBGameJamCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void ACMBGameJamCharacter::SubmitCommands(const TArray<EDroneCommand>& Commands)
{
	if (Commands.IsEmpty() || !GetWorld())
	{
		return;
	}

	// Every command starts CommandDelay after the previous one, counting all lists sent before it
	const double Now = GetWorld()->GetTimeSeconds();
	const double Delay = FMath::Max(0.f, CommandDelay);
	double ScheduledAt = FMath::Max(Now, LastScheduledTime);

	for (const EDroneCommand Command : Commands)
	{
		ScheduledAt += Delay;
		Pending.Add(FPendingCommand{ Command, ScheduledAt });
	}
	LastScheduledTime = ScheduledAt;

	// With no delay everything is already due
	ProcessPending(Now);
	UpdateTickState();
}

void ACMBGameJamCharacter::RequestStop()
{
	if (!GetWorld())
	{
		return;
	}

	const double Delay = FMath::Max(0.f, CommandDelay);
	if (Delay <= 0.0)
	{
		StopDrone();
		return;
	}

	// Stop doesn't line up behind pending commands: it lands after the delay and wipes them
	const double At = GetWorld()->GetTimeSeconds() + Delay;
	StopAt = StopAt < 0.0 ? At : FMath::Min(StopAt, At);
	UpdateTickState();
}

void ACMBGameJamCharacter::StopDrone()
{
	Pending.Reset();
	Active.Reset();
	LastScheduledTime = 0.0;
	StopAt = -1.0;

	StopJumping();
	JumpReleaseAt = -1.0;

	UpdateTickState();
}

void ACMBGameJamCharacter::ProcessPending(double Now)
{
	while (Pending.Num() > 0 && Pending[0].ExecuteAt <= Now)
	{
		const EDroneCommand Command = Pending[0].Command;
		Pending.RemoveAt(0);
		Activate(Command, Now);
	}
}

void ACMBGameJamCharacter::Activate(EDroneCommand Command, double Now)
{
	if (Command == EDroneCommand::Jump)
	{
		Jump();
		JumpReleaseAt = Now + 0.1;	// released a moment later so it doesn't re-jump on landing
		return;
	}

	const FDroneStep* Step = Steps.Find(Command);
	if (!Step)
	{
		return;
	}

	// A movement command cancels any turn in progress, so going from turning to just moving takes one input.
	// A turn command doesn't cancel movement, so the two can still stack in that order.
	const bool bIsMovement = !FMath::IsNearlyZero(Step->LocalMove.X) || !FMath::IsNearlyZero(Step->LocalMove.Y);
	if (bIsMovement)
	{
		for (int32 Index = Active.Num() - 1; Index >= 0; --Index)
		{
			FDroneStep& Other = Active[Index].Step;
			if (FMath::IsNearlyZero(Other.Turn))
			{
				continue;
			}

			if (Other.LocalMove.IsNearlyZero())
			{
				Active.RemoveAt(Index);
			}
			else
			{
				// A movement row that also turns (a curve) keeps moving, it just stops turning
				Other.Turn = 0.f;
			}
		}
	}

	// The new command replaces any active one on the same axis (Forward replaces Backward, and so on)
	Active.RemoveAll([Step](const FActiveAction& Other)
	{
		const FDroneStep& O = Other.Step;
		return (!FMath::IsNearlyZero(Step->LocalMove.X) && !FMath::IsNearlyZero(O.LocalMove.X))
			|| (!FMath::IsNearlyZero(Step->LocalMove.Y) && !FMath::IsNearlyZero(O.LocalMove.Y))
			|| (!FMath::IsNearlyZero(Step->Turn) && !FMath::IsNearlyZero(O.Turn));
	});

	Active.Add(FActiveAction{ *Step });
}

void ACMBGameJamCharacter::UpdateTickState()
{
	const bool bBusy = IsExecuting() || JumpReleaseAt >= 0.0;
	const bool bWasTicking = IsActorTickEnabled();

	SetActorTickEnabled(bBusy);

	if (bWasTicking && !bBusy)
	{
		OnFinished.Broadcast();
	}
}

void ACMBGameJamCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetWorld())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	if (JumpReleaseAt >= 0.0 && Now >= JumpReleaseAt)
	{
		StopJumping();
		JumpReleaseAt = -1.0;
	}

	// A stop that has come due cancels everything, including commands that haven't started
	if (StopAt >= 0.0 && Now >= StopAt)
	{
		StopDrone();
		return;
	}

	ProcessPending(Now);

	// Combine everything that is active: Forward + Right is a diagonal, a turn adds on top
	FVector MoveSum = FVector::ZeroVector;
	float TurnSum = 0.f;
	for (const FActiveAction& Action : Active)
	{
		MoveSum += FVector(Action.Step.LocalMove.X, Action.Step.LocalMove.Y, 0.f);
		TurnSum += Action.Step.Turn;
	}

	// Same entry points the template uses for player input: DoMove(Right, Forward) and DoAim(Yaw, Pitch)
	if (!MoveSum.IsNearlyZero())
	{
		const FVector Direction = MoveSum.GetSafeNormal();
		DoMove(Direction.Y, Direction.X);
	}

	// DoAim adds an amount per call, so scale by the frame time to get degrees per second
	if (!FMath::IsNearlyZero(TurnSum))
	{
		DoAim(FMath::Clamp(TurnSum, -1.f, 1.f) * TurnSpeed * DeltaTime, 0.f);
	}

	UpdateTickState();
}
