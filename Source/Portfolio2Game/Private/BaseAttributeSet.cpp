#include "BaseAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "CharacterBase.h"

// 생성자 정의
UBaseAttributeSet::UBaseAttributeSet()
{
	InitHealth(10.0f);
}

// GE가 적용된 직후 호출되는 함수
void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	ACharacterBase* OwnerChar = Cast<ACharacterBase>(GetOwningActor());
	if (!OwnerChar) return;

	// HP 또는 MaxHP가 변경되었는지 확인
	if (Data.EvaluatedData.Attribute == GetHPAttribute() || Data.EvaluatedData.Attribute == GetMaxHPAttribute())
	{
		// 1. (★수정★) HP가 MaxHP를 넘지 않도록 Clamp (내부 함수 사용)
		SetHealth_Internal(FMath::Min(GetHP(), GetMaxHP()));

		// 2. 델리게이트 방송! (BP_HPBarActor가 들을 수 있도록 int32로 형변환)
		OwnerChar->OnHealthChanged.Broadcast(FMath::RoundToInt(GetHP()), FMath::RoundToInt(GetMaxHP()));
	}
}