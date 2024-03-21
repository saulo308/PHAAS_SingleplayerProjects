// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorsCoordinator.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPSDActorsCoordinator, Log, All);

UCLASS()
class PHYSICSAAS_API APSDActorsCoordinator : public AActor
{
	GENERATED_BODY()
	
public:	 
	/** Sets default values for this actor's properties */
	APSDActorsCoordinator();

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** */
	UFUNCTION(BlueprintCallable)
	void StartPSDActorsSimulation();
	
	/** */
	UFUNCTION(BlueprintCallable)
	void StopPSDActorsSimulation();

	UFUNCTION(BlueprintCallable)
	void InitializeTest();

	UFUNCTION(BlueprintCallable)
	bool IsSimulating() { return bIsSimulating; }

	/** */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

private:

	void InitializePhysicsWorld();
	void UpdatePSDActors();

	void SaveFPSMeasureToFile();
	void SaveStepPhysicsMeasureToFile();

private:
	/** */
	TMap<uint32, class APSDActorBase*> PSDActorMap;

	bool bIsSimulating = false;

	class FPhysicsServiceImpl* PhysicsServiceLocalImpl = nullptr;

	FString CurrentDeltaTimeMeasure = FString();

	FString CurrentPhysicsStepSimulationTimeMeasure = FString();

	FTimerHandle TestTimerHandle;
};
