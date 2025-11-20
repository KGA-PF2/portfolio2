// Fill out your copyright notice in the Description page of Project Settings.


#include "NewFunctionLibrary.h"

void UNewFunctionLibrary::CalcTargetGridNum(int32 CurrentGridNum, EGridMoveDirection Direction, int32 Width, int32 Height, int32& CalcGridNum, bool& bSuccessOut)
{
    int32 TargetGridNum = CurrentGridNum;
    int32 TotalGrid = Width * Height;

    // 1. 입력된 방향(Enum)에 따라 Target 계산 (Up=+1, Down=-1, Left=-Height, Right=+Height 반영)
    switch (Direction)
    {
    case EGridMoveDirection::Up:
        TargetGridNum = CurrentGridNum - 1;
        break;
    case EGridMoveDirection::Down:
        TargetGridNum = CurrentGridNum + 1;
        break;
    case EGridMoveDirection::Left:
        TargetGridNum = CurrentGridNum - Height; // 상황에 따라 Width가 될 수도 있음 (GridISM 생성 조건에서 결정)
        break;
    case EGridMoveDirection::Right:
        TargetGridNum = CurrentGridNum + Height; // 상황에 따라 Width가 될 수도 있음 (GridISM 생성 조건에서 결정)
        break;
    }

    // 2. 조건 검사 0보다 크거나 같고, 전체 타일 수보다 작아야 함 (Index는 0부터 시작하므로 TotalGrid - 1)
    if (TargetGridNum >= 0 && TargetGridNum < TotalGrid)
    {
        // 범위 밖 처리 or 추가 조건 필요하게 될 경우 여기에 추가

        CalcGridNum = TargetGridNum;       // 이동할 인덱스 반환
        bSuccessOut = true;  // 성공 (True)를 의미
    }

    // 3. 조건 불만족 시 (변동 없음)
    else
    {
        CalcGridNum = CurrentGridNum; // 이동 실패 시 인덱스 유지
        bSuccessOut = false; // 실패 (False)를 의미
    }
}
