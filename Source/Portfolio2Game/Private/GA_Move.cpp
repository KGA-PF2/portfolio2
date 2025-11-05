#include "GA_Move.h"
#include "PlayerCharacter.h"    // (신규) APlayerCharacter의 EndAction(), bCanAct를 사용
#include "BattleManager.h"      // (신규) BattleManager의 좌표 계산 함수 사용
#include "GridDataInterface.h"  // (신규) BattleManager의 GridActorRef에서 그리드 크기를 가져오기 위함
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h" // (신규) 부드러운 이동을 위한 Latent Task

UGA_Move::UGA_Move()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Move::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 필요한 컴포넌트 가져오기
	APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!PlayerChar)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Move: APlayerCharacter가 아닌 대상이 어빌리티를 활성화했습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. (중요) 행동 가능(bCanAct) 상태인지 확인 (턴제 로직)
	if (!PlayerChar->bCanAct)
	{
		UE_LOG(LogTemp, Log, TEXT("GA_Move: bCanAct가 false이므로 이동할 수 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 캔슬됨 (턴 소모 X)
		return;
	}

	ABattleManager* BattleManager = PlayerChar->GetBattleManager();
	// (수정) BattleManagerRef->GridActorRef 접근 오류 수정
	if (!BattleManager || !BattleManager->GridActorRef || !BattleManager->GridActorRef->Implements<UGridDataInterface>())
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Move: BattleManager 또는 GridActorRef(Interface)를 찾을 수 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 목표 그리드 좌표(TargetCoord) 계산
	FIntPoint CurrentCoord = PlayerChar->GridCoord;
	FIntPoint TargetCoord = CurrentCoord;

	switch (MoveDirection)
	{
	case EGridDirection::Forward:
		TargetCoord.Y--;
		break;
	case EGridDirection::Backward:
		TargetCoord.Y++;
		break;
	case EGridDirection::Left:
		TargetCoord.X--;
		break;
	case EGridDirection::Right:
		TargetCoord.X++;
		break;
	}

	// 4. (Goal 3) 목표 좌표가 그리드 범위 내인지 유효성 검사
	const int32 GridWidth = IGridDataInterface::Execute_GetGridWidth(BattleManager->GridActorRef);
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(BattleManager->GridActorRef);

	if (TargetCoord.X < 0 || TargetCoord.X >= GridWidth || TargetCoord.Y < 0 || TargetCoord.Y >= GridHeight)
	{
		UE_LOG(LogTemp, Log, TEXT("GA_Move: 그리드 범위를 벗어난 이동입니다. (%d, %d)"), TargetCoord.X, TargetCoord.Y);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 캔슬됨 (턴 소모 X)
		return;
	}

	// 5. BattleManager에게 오프셋이 적용된 실제 월드 위치(TargetLocation) 요청
	FVector TargetLocation = BattleManager->GetWorldLocation(TargetCoord, true); // true = IsPlayer

	// 6. 캐릭터 내부 데이터(GridCoord) 즉시 업데이트
	PlayerChar->GridCoord = TargetCoord;

	// 7. 이동 애니메이션 재생 요청 (BP에서 구현)
	PlayerChar->PlayMoveAnim();

	// 8. (중요!) 부드러운 이동을 위한 AbilityTask(MoveToLocation) 생성

	// (수정 1) 이동 시간(Duration) 계산
	const FVector CurrentLocation = PlayerChar->GetActorLocation();
	const float Distance = FVector::Dist(CurrentLocation, TargetLocation);
	const float Duration = (MoveSpeed > 0) ? (Distance / MoveSpeed) : 0.f;

	// (수정 2) MoveToLocation 함수 시그니처 변경 (UE4 버전 호환)
	UAbilityTask_MoveToLocation* MoveTask = UAbilityTask_MoveToLocation::MoveToLocation(
		this,           // 어빌리티 자신
		NAME_None,      // 태스크 인스턴스 이름
		TargetLocation, // 목표 월드 위치
		Duration,       // (오류 수정) 4번째 인수는 '속도'가 아닌 '시간'입니다.
		nullptr,        // (오류 수정) 5번째 인수는 'float'가 아닌 'UCurveFloat*'입니다.
		nullptr         // (오류 수정) 6번째 인수는 'float'가 아닌 'UCurveVector*'입니다.
	);

	if (!MoveTask)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Move: MoveToLocation Task 생성 실패."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 9. 이동이 완료되면 OnMoveFinished 함수가 호출되도록 콜백(Delegate) 바인딩
	MoveTask->OnTargetLocationReached.AddDynamic(this, &UGA_Move::OnMoveFinished);

	// 10. 태스크 활성화 (이동 시작)
	MoveTask->ReadyForActivation();
}


void UGA_Move::OnMoveFinished()
{
	APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(GetCurrentActorInfo()->AvatarActor.Get());

	if (PlayerChar)
	{
		// 1. (중요) 플레이어의 턴을 종료합니다.
		PlayerChar->EndAction();
	}

	// 2. 어빌리티를 성공적으로 종료합니다.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false); // false = 캔슬 아님
}