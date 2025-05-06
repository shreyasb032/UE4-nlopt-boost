// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <nlopt.hpp>
#include <vector>
#include "Optimizer.generated.h"

typedef struct feedback_data {
	std::vector<int> performance;
	std::vector<int> trust_feedback;
	nlopt::opt _opt;
	int _max_evals;
};

UCLASS()
class UE4_NLOPT_API AOptimizer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOptimizer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void UpdateParameters(int performance, int trust_feedback, float alpha0_last, float beta0_last, float ws_last, float wf_last,
						 float& alpha0, float& beta0, float& ws, float& wf);

	UFUNCTION(BlueprintCallable)
	float GetTrustEstimate(float alpha0, float beta0, float ws, float wf);

	UFUNCTION(BlueprintCallable)
	void reset();

	std::vector<int> performance_history;
	std::vector<int> _trust_feedback;
	int MAX_EVAL;

private:
	void GetInitialGuess(int trust_feedback, float& alpha0, float& beta0, float& ws, float& wf);
};

double Objective(const std::vector<double>& x, std::vector<double>& grad, void* func_data);

