#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NiagaraComponent.h"
#include "TargetingComponent.generated.h"

class UNiagaraComponent;
class AActor;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CUBEWARS_API UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetingComponent();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    AActor* CurrentTarget;  // Stores the current target

    UPROPERTY()
    UNiagaraComponent* TargetEffect;  // Visual Indicator for Targeting

    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    float TargetingRange = 1000.f;  // Max range of targeting system

    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    TSubclassOf<AActor> TargetClass;  // The class of valid targets (e.g., enemies)

    FTimerHandle TargetValidationTimer;  // Timer for checking if the target is still valid

    void SelectNewTarget();
    void ValidateCurrentTarget();
    AActor* FindBestTarget();
    bool IsValidTarget(AActor* Target) const;

public:
    void CycleTarget();  // Call on Tab Press
};
