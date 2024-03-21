// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "PSDActorBase.generated.h"

UCLASS()
class PHYSICSAAS_API APSDActorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	/** Sets default values for this actor's properties */
	APSDActorBase();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	inline void ToggleSimulation(const bool bInShouldSimulate)
		{ bShouldSimulate = bInShouldSimulate; }

	void UpdatePositionAfterPhysicsSimulation(const FVector NewActorPosition);

	void UpdateRotationAfterPhysicsSimulation
		(const FVector NewActorRotationEulerAngles);

	FString GetInitialPosAsString();

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* ActorRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* ActorMesh = nullptr;

private:
	bool bShouldSimulate = false;
};
