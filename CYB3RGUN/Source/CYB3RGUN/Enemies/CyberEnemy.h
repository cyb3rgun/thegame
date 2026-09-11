// CYB3RGUN THEGAME. Base actor for every enemy.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyTypes.h"
#include "CyberEnemy.generated.h"

class UAnimSequenceBase;
class UEnemyDefinition;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEnemyDiedDelegate, ACyberEnemy*, Enemy, AController*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEnemyDamagedDelegate, ACyberEnemy*, Enemy, float, Amount, float, HealthFraction);

/**
 *  Base enemy. Health, one damage entry point, death, hit reaction hook, a definition for stats
 *  and a state tree driven by the AI controller for behaviour. The actor holds only what an
 *  instance must remember; every tunable lives in the definition so a later Mass Entity
 *  representation can read the same data.
 */
UCLASS()
class CYB3RGUN_API ACyberEnemy : public ACharacter
{
	GENERATED_BODY()

	/** Parent of the placeholder shapes, sits at the feet */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* PlaceholderRoot;

protected:

	/** Stats and visuals for this enemy. Set before FinishSpawning, or on the placed instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UEnemyDefinition> Definition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Target;

	float Health = 0.0f;
	bool bDead = false;
	float DeathPoseElapsed = 0.0f;
	float DeathPoseSeconds = 0.35f;
	FTimerHandle FlinchTimer;
	FTimerHandle LingerTimer;
	FTimerHandle BodyAnimationTimer;

	/** Loop the body plays right now, idle or move */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> BodyLoop;

	/** World time until which a one shot animation owns the body */
	float OneShotUntil = 0.0f;

public:

	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FEnemyDiedDelegate OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category="Enemy")
	FEnemyDamagedDelegate OnEnemyDamaged;

public:

	ACyberEnemy();

	/** Applies a definition. Call between deferred spawn and FinishSpawning, or before BeginPlay. */
	UFUNCTION(BlueprintCallable, Category="Enemy")
	void Initialize(UEnemyDefinition* InDefinition);

	/**
	 *  The one damage entry point. Weapons, fire and explosions all end here, either directly
	 *  or through the engine damage system, which routes into TakeDamage.
	 *  @return damage actually applied
	 */
	UFUNCTION(BlueprintCallable, Category="Enemy")
	float ApplyEnemyDamage(float Amount, EEnemyDamageSource Source, AController* EventInstigator, AActor* DamageCauser);

	/** Attacks the current target if it is in range. Returns true when damage was dealt. */
	UFUNCTION(BlueprintCallable, Category="Enemy")
	bool PerformAttack();

	UFUNCTION(BlueprintCallable, Category="Enemy")
	void SetTarget(AActor* InTarget) { Target = InTarget; }

	UFUNCTION(BlueprintCallable, Category="Enemy")
	void SetMoveSpeed(float Speed);

	UFUNCTION(BlueprintPure, Category="Enemy")
	AActor* GetTarget() const { return Target; }

	UFUNCTION(BlueprintPure, Category="Enemy")
	const UEnemyDefinition* GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintPure, Category="Enemy")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category="Enemy")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Enemy")
	float GetHealthFraction() const;

	/** Distance from this enemy to the target, or a large number without a target */
	UFUNCTION(BlueprintPure, Category="Enemy")
	float GetDistanceToTarget() const;

	/** True when the target is within the definition's attack range */
	UFUNCTION(BlueprintPure, Category="Enemy")
	bool IsTargetInAttackRange(float RangeScale = 1.0f) const;

	/** Point weapons should aim at, roughly the chest */
	UFUNCTION(BlueprintPure, Category="Enemy")
	FVector GetAimPoint() const;

	//~ Begin AActor
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor

protected:

	/** Blueprint hook for a hit reaction, called after the flinch */
	UFUNCTION(BlueprintImplementableEvent, Category="Enemy", meta = (DisplayName = "On Hit Reaction"))
	void BP_OnHitReaction(float Amount, EEnemyDamageSource Source);

	/** Blueprint hook for death, called after the death animation or the placeholder tip over starts */
	UFUNCTION(BlueprintImplementableEvent, Category="Enemy", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	void ApplyDefinition();

	/** True when a shot that lands at Location travelling along Direction passes through the head */
	bool IsHeadHit(const FVector& Location, const FVector& Direction) const;
	void BuildPlaceholder();

	/** True when the definition gives this enemy a skeletal body */
	bool HasBody() const;
	void BuildBody();

	/** Switches the body between idle and move by ground speed, on a short timer rather than every tick */
	void UpdateBodyAnimation();
	void PlayBodyOneShot(UAnimSequenceBase* Animation);
	void ClearPlaceholder();
	void Die(AController* Killer);
	void Flinch();
	void ClearFlinch();
	void RemoveBody();
	void PlayEnemySound(USoundBase* Sound) const;
};
