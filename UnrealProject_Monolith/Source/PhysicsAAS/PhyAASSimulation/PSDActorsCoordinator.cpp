// Fill out your copyright notice in the Description page of Project Settings.


#include "PSDActorsCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include <string>
#include <iostream>
#include <chrono>
#include "../PhysicsSimulation/PhysicsServiceImpl.h"
#include "PhysicsAAS/PhyAASSimulation/PSDActorBase.h"

DEFINE_LOG_CATEGORY(LogPSDActorsCoordinator);

APSDActorsCoordinator::APSDActorsCoordinator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APSDActorsCoordinator::BeginPlay()
{
	Super::BeginPlay();
}

void APSDActorsCoordinator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPSDActorsSimulation();
}

void APSDActorsCoordinator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSimulating)
	{
		// Update psda actors by simulating physics on the service
		// and parsing it's results with the new actor position
		UpdatePSDActors();

		// Append delta time measure
		CurrentDeltaTimeMeasure += FString::SanitizeFloat(DeltaTime) + "\n";
	}
}

void APSDActorsCoordinator::StartPSDActorsSimulation()
{
	UE_LOG(LogPSDActorsCoordinator, Warning, 
		TEXT("=> Starting PSD actors simulation."));

	// Get all PSDActors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
		APSDActorBase::StaticClass(), FoundActors);

	// Foreach found actor, append to PSDActorsList
	for (int i = 0; i < FoundActors.Num(); i++)
	{
		// Cast to PSDActor to get a reference to it
		auto PSDActor = Cast<APSDActorBase>(FoundActors[i]);
		if (!PSDActor)
		{
			continue;
		}

		// Add to map 
		// The key is the body id on the physics system. Starts at 1 as the
		// flor on physics system has body id of 0
		// The value is the reference to the actor
		PSDActorMap.Add(i + 1, PSDActor);
	}

	PhysicsServiceLocalImpl = new FPhysicsServiceImpl();
	if (!PhysicsServiceLocalImpl)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not create physics impl instance"));
		return;
	}

	// Initialize physics world
	InitializePhysicsWorld();

	// Foreach PSD actor, start simulation
	for (auto& PSDActor : PSDActorMap)
	{
		PSDActor.Value->ToggleSimulation(true);
	}

	CurrentDeltaTimeMeasure = FString();
	CurrentPhysicsStepSimulationTimeMeasure = FString();

	bIsSimulating = true;

	UE_LOG(LogPSDActorsCoordinator, Log, 
		TEXT("PSD actors started simulating..."));
}

void APSDActorsCoordinator::StopPSDActorsSimulation()
{
	if (!bIsSimulating)
	{
		return;
	}

	UE_LOG(LogPSDActorsCoordinator, Warning, 
		TEXT("=> Stopping PSD actors simulation."));

	bIsSimulating = false;

	// Foreach PSD actor, stop simulation
	for (auto& PSDActor : PSDActorMap)
	{
		PSDActor.Value->ToggleSimulation(false);
	}

	// Save FPS string measure and Step physics measure into file
	SaveFPSMeasureToFile();
	SaveStepPhysicsMeasureToFile();

	UE_LOG(LogPSDActorsCoordinator, Log, 
		TEXT("PSD actors stopped simulating..."));
}

void APSDActorsCoordinator::InitializeTest()
{
	// Start the PSD actors simulation
	StartPSDActorsSimulation();

	// Start timer to stop the simulation after 30 seconds passed
	GetWorld()->GetTimerManager().SetTimer(TestTimerHandle, this, 
		&APSDActorsCoordinator::StopPSDActorsSimulation, 30.f, false);
}
               
void APSDActorsCoordinator::InitializePhysicsWorld()
{
	// Start initialization message
	FString InitializationMessage = "Init\n";

	// Foreach PSD actor, get its StepPhysicsString
	for (auto& PSDActor : PSDActorMap)
	{
		InitializationMessage += FString::Printf(TEXT("%d;%s\n"), 
			PSDActor.Key, *PSDActor.Value->GetInitialPosAsString());
	}

	// Convert message to std string
	std::string InitializationMessageAsStdString
		(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* InitializationMessageAsChar = &InitializationMessageAsStdString[0];

	PhysicsServiceLocalImpl->InitPhysicsSystem(InitializationMessageAsChar);
}	

void APSDActorsCoordinator::UpdatePSDActors()
{
	// Check if we are simulating
	if (!bIsSimulating)
	{
		return;
	}

	// Get pre step physics time
	std::chrono::steady_clock::time_point preStepPhysicsTime = 
		std::chrono::steady_clock::now();

	// Step physics
	FString PhysicsSimulationResultStr = 
		PhysicsServiceLocalImpl->StepPhysicsSimulation();

	// Get post physics time
	std::chrono::steady_clock::time_point postStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Calculate the microsseconds step physics simulation took
	std::stringstream ss;
	ss << std::chrono::duration_cast<std::chrono::microseconds>(postStepPhysicsTime - preStepPhysicsTime).count();
	const std::string elapsedTime = ss.str();

	// Get the delta time in FString
	const FString ElapsedPhysicsTimeMicroseconds = 
		UTF8_TO_TCHAR(elapsedTime.c_str());

	// Append the delta time to the current step measurement
	CurrentPhysicsStepSimulationTimeMeasure += 
		ElapsedPhysicsTimeMicroseconds + "\n";

	//UE_LOG(LogTemp, Warning, TEXT("Physics service response: %s"), 
		//*PhysicsSimulationResultStr);

	// Parse physics simulation result
	// Each line will contain a result for a actor in terms of:
	// "Id; posX; posY; posZ; rotX; rotY; rotZ"
	TArray<FString> ParsedSimulationResult;
	PhysicsSimulationResultStr.ParseIntoArrayLines(ParsedSimulationResult);

	// Foreach line, parse its results (getting each actor pos)
	for (auto& SimulationResultLine : ParsedSimulationResult)
	{
		// Parse the line with ";" delimit
		TArray<FString> ParsedActorSimulationResult;
		SimulationResultLine.ParseIntoArray(ParsedActorSimulationResult, 
			TEXT(";"), false);

		// Check for errors
		if (ParsedActorSimulationResult.Num() < 7)
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not parse line %s. Num is:%d"),
				*SimulationResultLine,ParsedSimulationResult.Num());
			return;
		}

		// Get the actor id to float
		const uint32 ActorID = FCString::Atoi(*ParsedActorSimulationResult[0]);

		// Find the actor on the map
		APSDActorBase* ActorToUpdate = PSDActorMap[ActorID];
		if (!ActorToUpdate) 
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find actor with ID:%f"),
				ActorID);

			continue;
		}

		// Update PSD actor position with the result
		const float NewPosX = FCString::Atof(*ParsedActorSimulationResult[1]);
		const float NewPosY = FCString::Atof(*ParsedActorSimulationResult[2]);
		const float NewPosZ = FCString::Atof(*ParsedActorSimulationResult[3]);
		const FVector NewPos(NewPosX, NewPosY, NewPosZ);

		ActorToUpdate->UpdatePositionAfterPhysicsSimulation(NewPos);

		// Update PSD actor rotation with the result
		const float NewRotX = FCString::Atof(*ParsedActorSimulationResult[4]);
		const float NewRotY = FCString::Atof(*ParsedActorSimulationResult[5]);
		const float NewRotZ = FCString::Atof(*ParsedActorSimulationResult[6]);
		const FVector NewRotEuler(NewRotX, NewRotY, NewRotZ);

		ActorToUpdate->UpdateRotationAfterPhysicsSimulation(NewRotEuler);
	}
}

void APSDActorsCoordinator::SaveFPSMeasureToFile()
{
	FString TargetFolder = TEXT("FPSMeasure");
	FString FullFolderPath = 
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"), 
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	int32 FileCount = 1;
	FString FileName = FString::Printf(TEXT("/FPSMeasure_%dSpheres_%d.txt"), 
		PSDActorMap.Num(), FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder + 
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/FPSMeasure_%dSpheres_%d.txt"), 
			PSDActorMap.Num(), FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	UE_LOG(LogTemp, Warning, TEXT("Saving into %s"), *FileFullPath);
	FFileHelper::SaveStringToFile(CurrentDeltaTimeMeasure, *FileFullPath);
}

void APSDActorsCoordinator::SaveStepPhysicsMeasureToFile()
{
	FString TargetFolder = TEXT("StepPhysicsMeasure");
	FString FullFolderPath =
		FString(FPlatformProcess::UserDir() + TargetFolder);

	FullFolderPath = FullFolderPath.Replace(TEXT("/"), TEXT("\\\\"));

	//Criando diretório se já não existe
	if (!IFileManager::Get().DirectoryExists(*FullFolderPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Criando diretorio: %s"),
			*FullFolderPath);
		IFileManager::Get().MakeDirectory(*FullFolderPath);
	}

	int32 FileCount = 1;
	FString FileName = 
		FString::Printf(TEXT("/StepPhysicsMeasure_%dSpheres_%d.txt"),
		PSDActorMap.Num(), FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = 
			FString::Printf(TEXT("/StepPhysicsMeasure_%dSpheres_%d.txt"),
			PSDActorMap.Num(), FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	UE_LOG(LogTemp, Warning, TEXT("Saving into %s"), *FileFullPath);
	FFileHelper::SaveStringToFile(CurrentPhysicsStepSimulationTimeMeasure, 
		*FileFullPath);
}
