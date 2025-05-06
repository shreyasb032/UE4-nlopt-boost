// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParticipantSimulator.generated.h"

UCLASS()
class CSVDATAREADER_API AParticipantSimulator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AParticipantSimulator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Set the data for reading the files
	UFUNCTION(BlueprintCallable)
	void SetSelectorData(int pid, int mission_num, int interaction_mode);

	// Reads the data from the file and converts it into arrays
	UFUNCTION(BlueprintCallable)
	bool ReadData();

	// Gets one line of data needed for the parameter updater
	UFUNCTION(BlueprintCallable)
	void GetData(int site_idx, int& performance, int& trust_feedback);


	UPROPERTY(BlueprintReadOnly)
	int num_sites;

	UFUNCTION(BlueprintCallable)
	bool WriteTrustEstimates(const TArray<float>& new_estimates);


private:
	// Data needed for selecting the file
	int _pid;
	int _mission_num;
	int _interaction_mode;
	TArray<FString> mode_names = { "Constant", "State dependent" };
	FString _mode_name;
	FString _base_dir;

	// Set the base directory of the data files
	void SetDirectory();

	// Convert the file data string to desired arrays
	void ConvertDataToArrays();

	// Data to read
	TArray<int> house_numbers;
	TArray<int> health_points;
	TArray<int> time_points;
	TArray<int> recommendations;
	TArray<int> _trust_feedback;
	TArray<float> original_estimates;
	TArray<int> performance_history;
	TArray<float> threat_levels;
	FString data;
	TArray<FString> lines;

	void reset();
};
