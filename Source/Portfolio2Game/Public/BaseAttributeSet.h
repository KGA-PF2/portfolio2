#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"

// HP 변경을 ACharacterBase에 알릴 델리게이트 (int32로 수정)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedDelegate, int32 /*NewHealth*/, int32 /*NewMaxHealth*/);

// (★필수★) C++ 컴파일을 위한 매크로
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	// 생성자 선언
	UBaseAttributeSet();

	// CharacterBase가 구독할 델리게이트 인스턴스
	FOnHealthChangedDelegate OnHealthChanged;

	// === HP ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HP);

	// === Max HP ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHP);

	// 블루프린트 전용 Getter 함수 (이름 변경)
	UFUNCTION(BlueprintPure, Category = "Attributes", meta = (DisplayName = "GetHealth (float)"))
	float GetHealth_BP() const { return HP.GetCurrentValue(); }

	UFUNCTION(BlueprintPure, Category = "Attributes", meta = (DisplayName = "GetMaxHealth (float)"))
	float GetMaxHealth_BP() const { return MaxHP.GetCurrentValue(); }

	//  C++ 내부용 Setter 함수 (이름 변경)
	void SetHealth_Internal(float NewVal) { HP.SetCurrentValue(NewVal); }
	void SetMaxHealth_Internal(float NewVal) { MaxHP.SetCurrentValue(NewVal); }

public:
	// === 체력 관련 유틸 ===
	FORCEINLINE bool IsDead() const
	{
		return HP.GetCurrentValue() <= 0.f;
	}

	// 체력 초기화
	FORCEINLINE void InitHealth(float NewMaxHP)
	{
		MaxHP.SetBaseValue(NewMaxHP);
		MaxHP.SetCurrentValue(NewMaxHP);
		HP.SetBaseValue(NewMaxHP);
		HP.SetCurrentValue(NewMaxHP);
	}

	// 데미지 적용 (BP_EnemyCharacter가 호출하므로 UFUNCTION 추가)
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	FORCEINLINE void ApplyDamage(float Damage)
	{
		const float NewHP = FMath::Clamp(HP.GetCurrentValue() - Damage, 0.f, MaxHP.GetCurrentValue());
		HP.SetCurrentValue(NewHP);
	}

	// 회복
	FORCEINLINE void Heal(float Amount)
	{
		const float NewHP = FMath::Clamp(HP.GetCurrentValue() + Amount, 0.f, MaxHP.GetCurrentValue());
		HP.SetCurrentValue(NewHP);
	}

	// PostGameplayEffectExecute 선언
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};