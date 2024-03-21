// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PSDActorsSpawner.generated.h"

UCLASS()
class PHYSICSAAS_API APSDActorsSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APSDActorsSpawner();

	UFUNCTION(BlueprintCallable)
	void SpawnPSDActors(const int32 NumberOfActorsToSpawn);

	UFUNCTION(BlueprintCallable)
	void DestroyPSDActors();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class APSDActorBase> ActorToSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxXPos = 1100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MaxYPos = -1100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float MinZPos = 220.f;
};
