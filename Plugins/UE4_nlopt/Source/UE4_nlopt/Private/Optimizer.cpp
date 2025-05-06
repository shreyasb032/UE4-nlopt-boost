// Fill out your copyright notice in the Description page of Project Settings.


#include "Optimizer.h"
#include <boost/math/special_functions/digamma.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math_fwd.hpp>
#include <exception>

// Sets default values
AOptimizer::AOptimizer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	MAX_EVAL = 10;
}

// Called when the game starts or when spawned
void AOptimizer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOptimizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


double Objective(const std::vector<double> &x, std::vector<double> &grad, void* func_data)
{
	double _alpha0 = x[0];
	double _beta0 = x[1];
	double _ws = x[2];
	double _wf = x[3];
	double logl = 0.f;

	double alpha = _alpha0;
	double beta = _beta0;
	int ns = 0, nf = 0;

	feedback_data* data = reinterpret_cast<feedback_data*>(func_data);
	std::vector<int> performance_history = data->performance;
	std::vector<int> _trust_feedback = data->trust_feedback;
	nlopt::opt opt = data->_opt;
	int max_evals = data->_max_evals;
	if (opt.get_numevals() >= max_evals)
	{
		opt.force_stop();
	}

	/*if (_trust_feedback.size() == 7)
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, FString::Printf(TEXT("Performance: %d, Feedback: %d"), performance_history.back(), _trust_feedback.back()));*/

	for (int i = 0; i < performance_history.size(); i++)
	{
		int p = performance_history[i];
		double t = (double)_trust_feedback[i] / 100.;
		if (t < 0.01) t = 0.01;
		if (t > 0.99) t = 0.99;
		ns += p;
		nf += (1 - p);
		alpha += p * _ws;
		beta += (1 - p) * _wf;
		double logt = (double) FMath::Loge(t);
		double log1t = (double) FMath::Loge(1. - t);
		logl += boost::math::lgamma(alpha + beta) - boost::math::lgamma(alpha) - boost::math::lgamma(beta);
		logl += (alpha - 1) * logt + (beta - 1) * log1t;

		double digamma_both = boost::math::digamma(alpha + beta);
		double digamma_alpha = boost::math::digamma(alpha);
		double digamma_beta = boost::math::digamma(beta);

		grad[0] += (digamma_both - digamma_alpha + logt);
		grad[1] += (digamma_both - digamma_beta + log1t);
		grad[2] += (digamma_both - digamma_alpha + logt) * ns;
		grad[3] += (digamma_both - digamma_beta + log1t) * nf;
	}

	/*if (_trust_feedback.size() == 7) {
		FString out = FString::Printf(TEXT("grad[0]: %.2f, grad[1]: %.2f, grad[2]: %.2f, grad[3]: %.2f"), grad[0], grad[1], grad[2], grad[3]);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, out);
	}*/

	return logl;
}


//double AOptimizer::Objective(const std::vector<double>& x, std::vector<double>& grad, void* func_data)
//{
//	float _alpha0 = x[0];
//	float _beta0 = x[1];
//	float _ws = x[2];
//	float _wf = x[3];
//	float logl = 0.f;
//
//	float alpha = _alpha0;
//	float beta = _beta0;
//	int ns = 0, nf = 0;
//
//	for (int i = 0; i < performance_history.size(); i++)
//	{
//		int p = performance_history[i];
//		float t = (float)_trust_feedback[i] / 100.f;
//		if (t < 0.01) t = 0.01f;
//		if (t > 0.99) t = 0.99f;
//		ns += p;
//		nf += p;
//		alpha += p * _ws;
//		beta += (1 - p) * _wf;
//		float logt = FMath::Loge(t);
//		float log1t = FMath::Loge(1.f - t);
//		logl += boost::math::lgamma(alpha + beta) - boost::math::lgamma(alpha) - boost::math::lgamma(beta);
//		logl += (alpha - 1) * logt + (beta - 1) * log1t;
//
//		float digamma_both = boost::math::digamma(alpha + beta);
//		float digamma_alpha = boost::math::digamma(alpha);
//		float digamma_beta = boost::math::digamma(beta);
//
//		grad[0] += (digamma_both - digamma_alpha + logt);
//		grad[1] += (digamma_both - digamma_beta + log1t);
//		grad[2] += (digamma_both - digamma_alpha + logt) * ns;
//		grad[3] += (digamma_both - digamma_beta + log1t) * nf;
//	}
//
//	return -logl;
//}


void AOptimizer::UpdateParameters(int performance, int trust_feedback, float alpha0_last, float beta0_last, float ws_last, float wf_last,
								  float& alpha0, float& beta0, float& ws, float& wf)
{
	// Add the data
	performance_history.push_back(performance);
	_trust_feedback.push_back(trust_feedback);

	if (performance_history.size() < 2)
	{
		GetInitialGuess(trust_feedback, alpha0, beta0, ws, wf);
		return;
	}
	
	// Setup the optimizer
	nlopt::opt opt(nlopt::LD_LBFGS, 4);

	// Set the lower bounds
	std::vector<double> lb = { 1., 1., 0.1, 0.1 };
	opt.set_lower_bounds(lb);

	// Set the upper bounds
	std::vector<double> ub = { 200., 200., 200., 200. };
	opt.set_upper_bounds(ub);

	// Stopping criteria
	opt.set_maxeval(15);
	opt.set_xtol_rel(1e-1);
	opt.set_ftol_rel(1e-1);
	opt.set_ftol_abs(1e-4);
	//opt.set_xtol_abs(1e-4);
	
	// Choose an initial guess
	std::vector<double> x0 = { alpha0_last, beta0_last, ws_last, wf_last };
	
	// Add the external data
	feedback_data data;
	data.performance = performance_history;
	data.trust_feedback = _trust_feedback;
	data._opt = opt;
	data._max_evals = MAX_EVAL;

	// Set the objective function
	opt.set_max_objective(Objective, &data);

	// Add a variable for the minimum function value
	double minf;

	try {
		nlopt::result result = opt.optimize(x0, minf);
		alpha0 = static_cast<float> (x0[0]);
		beta0 = static_cast<float> (x0[1]);
		ws = static_cast<float> (x0[2]);
		wf = static_cast<float> (x0[3]);
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("Solved! Num Evals: %d"), opt.get_numevals()));
	}
	catch (...) {
		alpha0 = static_cast<float> (x0[0]);
		beta0 = static_cast<float> (x0[1]);
		ws = static_cast<float> (x0[2]);
		wf = static_cast<float> (x0[3]);
		//FString out = FString::Printf(TEXT("%s, Num Evals: %d"), e.what(), opt.get_numevals());
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, out);

	}
}


void AOptimizer::GetInitialGuess(int trust_feedback, float& alpha0, float& beta0, float& ws, float& wf)
{
	ws = 1.f;
	wf = 2.f;
	alpha0 = trust_feedback;
	if (alpha0 <= 1) alpha0 = 1.1;
	if (alpha0 >= 99) alpha0 = 98.9;
	beta0 = 100 - alpha0;
}

float AOptimizer::GetTrustEstimate(float alpha0, float beta0, float ws, float wf)
{
	float alpha = alpha0, beta = beta0;
	for (int p : performance_history)
	{
		alpha += p * ws;
		beta += (1 - p) * wf;
	}
	return alpha / (alpha + beta);
}

void AOptimizer::reset()
{
	_trust_feedback.clear();
	performance_history.clear();
}