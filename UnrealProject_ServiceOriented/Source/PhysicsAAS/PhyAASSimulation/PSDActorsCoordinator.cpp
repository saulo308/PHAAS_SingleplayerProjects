// Fill out your copyright notice in the Description page of Project Settings.


#include "PSDActorsCoordinator.h"
#include "PhysicsAAS/Proxies/SocketClientProxy.h"
#include "Kismet/GameplayStatics.h"
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
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

void APSDActorsCoordinator::StartPSDActorsSimulation
	(const FString& SocketServerIpAddr)
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

	// Open Socket connection with localhost server
	const bool bWasOpenSocketSuccess =
		FSocketClientProxy::OpenSocketConnection(SocketServerIpAddr, PSDActorMap.Num());

	// Check for errors on opening. 
	// Any error should be shown in log
	if (!bWasOpenSocketSuccess)
	{
		UE_LOG(LogPSDActorsCoordinator, Error,
			TEXT("Socket openning error. Check logs."));
		return;
	}

	UE_LOG(LogPSDActorsCoordinator, Log, TEXT("Socket opened..."));

	// Initialize physics world
	InitializePhysicsWorld();

	// Foreach PSD actor, start simulation
	for (auto& PSDActor : PSDActorMap)
	{
		PSDActor.Value->ToggleSimulation(true);
	}

	CurrentDeltaTimeMeasure = FString();
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

	// Close Socket connection with localhost server
	const bool bWaCloseSocketSuccess = 
		FSocketClientProxy::CloseSocketConnection();

	// Check for errors on opening. 
	// Any error should be shown in log
	if (!bWaCloseSocketSuccess)
	{
		UE_LOG(LogPSDActorsCoordinator, Error, 
			TEXT("Socket closing error. Check logs."));
		return;
	}

	UE_LOG(LogPSDActorsCoordinator, Log, TEXT("Socket closed..."));

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

void APSDActorsCoordinator::InitializeTest(const FString& SocketServerIpAddr)
{
	// Start the PSD actors simulation
	StartPSDActorsSimulation(SocketServerIpAddr);

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

	InitializationMessage += "EndMessage\n";

	// Convert message to std string
	std::string MessageAsStdString(TCHAR_TO_UTF8(*InitializationMessage));

	// Convert message to char*. This is needed as some UE converting has the
	// limitation of 128 bytes, returning garbage when it's over it
	char* MessageAsChar = &MessageAsStdString[0];

	// Send message to initialize physics world on service
	const FString Response = FSocketClientProxy::SendMessageAndGetResponse
		(MessageAsChar);
}	

void APSDActorsCoordinator::UpdatePSDActors()
{
	// Check if we are simulating
	if (!bIsSimulating)
	{
		return;
	}

	UE_LOG(LogTemp, Warning ,TEXT("Updating..."));

	// Check if we have a valid connection
	if (!FSocketClientProxy::HasValidConnection())
	{
		UE_LOG(LogTemp, Error, 
			TEXT("Could not simulate as there's no valid connection"));
		return;
	}

	// Get pre step physics time
	std::chrono::steady_clock::time_point preStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Construct message. This will be verified so service knows that we are
	// making a "step physics" call
	const char* StepPhysicsMessage = "Step";

	// Request physics simulation on physics service bt passing the 
	// world delta time
	FString PhysicsSimulationResultStr = 
		FSocketClientProxy::SendMessageAndGetResponse(StepPhysicsMessage);

	// Get post physics communication time
	std::chrono::steady_clock::time_point postStepPhysicsTime =
		std::chrono::steady_clock::now();

	// Calculate the microsseconds all step physics simulation
	// (considering communication )took
	std::stringstream ss;
	ss << std::chrono::duration_cast<std::chrono::microseconds>(postStepPhysicsTime - preStepPhysicsTime).count();
	const std::string elapsedTime = ss.str();

	// Get the delta time in FString
	const FString ElapsedPhysicsTimeMicroseconds =
		UTF8_TO_TCHAR(elapsedTime.c_str());

	// Append the delta time to the current step measurement
	CurrentPhysicsStepSimulationWithCommsTimeMeasure +=
		ElapsedPhysicsTimeMicroseconds + "\n";

	//UE_LOG(LogTemp, Warning, TEXT("Physics service response: %s"), *PhysicsSimulationResultStr);
	//UE_LOG(LogTemp, Warning, TEXT("=============="));

	// Parse physics simulation result
	// Each line will contain a result for a actor in terms of:
	// "Id; posX; posY; posZ; rotX; rotY; rotZ"
	TArray<FString> ParsedSimulationResult;
	PhysicsSimulationResultStr.ParseIntoArrayLines(ParsedSimulationResult);

	// Foreach line, parse its results (getting each actor pos)
	for (auto& SimulationResultLine : ParsedSimulationResult)
	{
		// Check if the line is just "OK" message
		if (SimulationResultLine.Contains("OK"))
		{
			continue;
		}

		// Parse the line with ";" delimit
		TArray<FString> ParsedActorSimulationResult;
		SimulationResultLine.ParseIntoArray(ParsedActorSimulationResult, 
			TEXT(";"), false);

		// Check for errors
		if (ParsedActorSimulationResult.Num() < 7)
		{
			UE_LOG(LogTemp, Error, TEXT("Could not parse line %s. Num is:%d"),
				*SimulationResultLine, ParsedActorSimulationResult.Num());
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

		// Update PSD actor with the result
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
	FString FileName = FString::Printf(TEXT("/FPSMeasure_Remote_%dSpheres_%d.txt"),
		PSDActorMap.Num(), FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName = FString::Printf(TEXT("/FPSMeasure_Remote_%dSpheres_%d.txt"),
			PSDActorMap.Num(), FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	UE_LOG(LogTemp, Warning, TEXT("Saving into %s"), *FileFullPath);
	FFileHelper::SaveStringToFile(CurrentDeltaTimeMeasure, *FileFullPath);
}

void APSDActorsCoordinator::SaveStepPhysicsMeasureToFile()
{
	FString TargetFolder = TEXT("StepPhysicsMeasureWithCommsOverhead");
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
		FString::Printf(TEXT("/StepPhysicsMeasureWithCommsOverhead_%dSpheres_%d.txt"),
			PSDActorMap.Num(), FileCount);

	FString FileFullPath = FPlatformProcess::UserDir() + TargetFolder +
		FileName;

	while (IFileManager::Get().FileExists(*FileFullPath))
	{
		FileCount++;
		FileName =
			FString::Printf(TEXT("/StepPhysicsMeasureWithCommsOverhead_%dSpheres_%d.txt"),
				PSDActorMap.Num(), FileCount);

		FileFullPath = FPlatformProcess::UserDir() + TargetFolder + FileName;
	}

	UE_LOG(LogTemp, Warning, TEXT("Saving into %s"), *FileFullPath);
	FFileHelper::SaveStringToFile(CurrentPhysicsStepSimulationWithCommsTimeMeasure,
		*FileFullPath);
}
