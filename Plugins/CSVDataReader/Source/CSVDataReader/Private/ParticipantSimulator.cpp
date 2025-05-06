// Fill out your copyright notice in the Description page of Project Settings.


#include "ParticipantSimulator.h"

// Sets default values
AParticipantSimulator::AParticipantSimulator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	_pid = -1;
	_mission_num = -1;
	_interaction_mode = -1;
	num_sites = -1;
}

// Called when the game starts or when spawned
void AParticipantSimulator::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AParticipantSimulator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AParticipantSimulator::SetDirectory()
{
	FString project_dir = FPaths::ProjectDir();
	FString p_identifier = "Participant0";
	if (_pid < 10) p_identifier += "0";
	p_identifier += FString::FromInt(_pid);
	FString mission_identifier = "Mission" + FString::FromInt(_mission_num);
	FString csv = "CSV";
	_base_dir = FPaths::Combine(project_dir, csv, p_identifier, _mode_name, mission_identifier);
}

void AParticipantSimulator::SetSelectorData(int pid, int mission_num, int interaction_mode)
{
	_pid = pid;
	_mission_num = mission_num;
	_interaction_mode = interaction_mode;
	_mode_name = mode_names[_interaction_mode];
	SetDirectory();
}


bool AParticipantSimulator::ReadData()
{
	reset();
	FString filename = "Data.csv";
	FString filepath = FPaths::Combine(_base_dir, filename);
	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, filename);
	bool read = FFileHelper::LoadFileToString(data, *filepath);
	ConvertDataToArrays();
	num_sites = _trust_feedback.Num();
	return read;
}

void AParticipantSimulator::ConvertDataToArrays()
{
	int num_lines = data.ParseIntoArrayLines(lines);

	for (int i = 1; i < num_lines; i++)
	{
		FString line = lines[i];
		TArray<FString> values;
		FString sep = ",";
		int num_vals = line.ParseIntoArray(values, *sep);
		house_numbers.Push(FCString::Atoi(*values[0]));
		health_points.Push(FCString::Atoi(*values[7]));
		time_points.Push(FCString::Atoi(*values[9]));
		recommendations.Push(FCString::Atoi(*values[11]));
		_trust_feedback.Push(FCString::Atoi(*values[12]));
		original_estimates.Push(FCString::Atof(*values[13]));
		performance_history.Push(FCString::Atoi(*values[14]));
		threat_levels.Push(FCString::Atof(*values[16]));
	}
	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::FromInt(num_lines));
}

void AParticipantSimulator::GetData(int site_idx, int& performance, int& trust_feedback)
{
	performance = performance_history[site_idx];
	trust_feedback = _trust_feedback[site_idx];
}

bool AParticipantSimulator::WriteTrustEstimates(const TArray<float>& new_estimates)
{
	FString filename = "NewTrustEstimates.csv";
	FString filepath = FPaths::Combine(_base_dir, filename);
	FString _data = "TrustFeedback,OriginalEstimate,NewEstimate";
	_data += LINE_TERMINATOR;
	for (int i = 0; i < new_estimates.Num(); i++)
	{
		_data += FString::FromInt(_trust_feedback[i]) + ",";
		_data += FString::SanitizeFloat(original_estimates[i]) + ",";
		_data += FString::SanitizeFloat(new_estimates[i]);
		_data += LINE_TERMINATOR;
	}
	return FFileHelper::SaveStringToFile(_data, *filepath);
}


void AParticipantSimulator::reset()
{
	house_numbers.Empty();
	health_points.Empty();
	time_points.Empty();
	recommendations.Empty();
	_trust_feedback.Empty();
	original_estimates.Empty();
	performance_history.Empty();
	threat_levels.Empty();
}