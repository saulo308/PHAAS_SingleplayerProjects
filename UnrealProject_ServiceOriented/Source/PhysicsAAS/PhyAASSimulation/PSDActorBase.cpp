// Fill out your copyright notice in the Description page of Project Settings.


#include "PSDActorBase.h"

APSDActorBase::APSDActorBase()
{
	PrimaryActorTick.bCanEverTick = true;

	ActorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ActorRoot"));
	RootComponent = ActorRoot;

	ActorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActorMesh"));
	ActorMesh->SetupAttachment(ActorRoot);
}

void APSDActorBase::BeginPlay()
{
	Super::BeginPlay();
}

void APSDActorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FString APSDActorBase::GetInitialPosAsString()
{
	const FVector ActorPos = GetActorLocation();

	const FString StepPhysicsString =
		FString::Printf(TEXT("%f;%f;%f"), ActorPos.X, ActorPos.Y, 
		ActorPos.Z);

	return StepPhysicsString;
}

void APSDActorBase::UpdatePositionAfterPhysicsSimulation
	(const FVector NewActorPosition)
{
	SetActorLocation(NewActorPosition);
}

void APSDActorBase::UpdateRotationAfterPhysicsSimulation
	(const FVector NewActorRotationEulerAngles)
{
	FQuat NewRotation = FQuat::MakeFromEuler(NewActorRotationEulerAngles);
	SetActorRotation(NewRotation);
}
