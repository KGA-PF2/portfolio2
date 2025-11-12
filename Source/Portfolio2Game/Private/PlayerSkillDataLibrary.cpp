#include "PlayerSkillDataLibrary.h"
#include "SkillBase.h" 

int32 UPlayerSkillDataLibrary::GetEffectiveDamage(const FPlayerSkillData& SkillData)
{
	if (!SkillData.SkillInfo) return 0;
	return FMath::Max(0, SkillData.SkillInfo->BaseDamage + SkillData.DamageDelta);
}

int32 UPlayerSkillDataLibrary::GetEffectiveTotalCooldown(const FPlayerSkillData& SkillData)
{
	if (SkillData.TotalCooldown > 0) return SkillData.TotalCooldown;
	if (!SkillData.SkillInfo) return 0;
	return FMath::Max(0, SkillData.SkillInfo->BaseCooldown + SkillData.CooldownDelta);
}

FName UPlayerSkillDataLibrary::GetSkillName(const FPlayerSkillData& SkillData)
{
	// PlayerSkillData.h의 GetSkillName() 로직을 그대로 사용
	return SkillData.SkillInfo ? SkillData.SkillInfo->SkillName : "None";
}