#include <iostream>
using namespace std;

#define MAX 1024  // 배열 크기를 넉넉하게 잡기 위한 상수

struct Implicant {
    int value;
    int mask; 
    bool used;
    bool isDC; 
    int minterms[MAX];
    int mCount;
};

int num; // 변수의 총 개수 (예: 4이면 x1~x4)
int fCount; // f=1인 최소항의 개수
int dcCount; // Don't Care 최소항의 개수
int fMinterms[MAX]; // f=1인 최소항 번호들을 저장하는 배열
int dcMinterms[MAX]; // Don't Care 최소항 번호들을 저장하는 배열

Implicant PIs[MAX]; // 최종 Prime Implicant(주항)들을 저장하는 배열
int piCount = 0; // 찾아낸 PI의 총 개수

Implicant curList[MAX]; // 현재 비교 대상인 항 목록
Implicant nextList[MAX];  // 병합 결과로 새로 생성된 항 목록

int coverTable[MAX][MAX]; // PI(행) × 최소항(열) 커버 여부 표 (1이면 커버)
Implicant answer[MAX];
int ansCount = 0;

 // 이진수 표현에서 1의 개수를 세는 함수
int countOnes(int x){
    int count = 0;

    while (x > 0){
        // x의 맨 오른쪽 비트가 1인지 확인
        if ((x & 1) == 1){
            count = count + 1;
        }

        // x를 오른쪽으로 1비트 이동
        x = x >> 1;
    }

    // 최종적인 count 값 반환
    return count;
}


// 두 Implicant가 같은지 비교하는 함수
bool isSame(Implicant& a, Implicant& b){
    return (a.mask == b.mask) && (a.value == b.value);
}


// 중복 생성된 항을 병합하는 함수
void mergeMinterms(Implicant& a, Implicant& b){
    for (int i = 0; i < a.mCount; i++){
        bool exist = false;
        for (int j = 0; j < b.mCount; j++){
            if (b.minterms[j] == a.minterms[i]) {
                exist = true; // 이미 있는 경우는 확인할 필요 없음
                break;
            }
        }

        // b에 없는 최소항이면 끝에 추가
        if (!exist){
            b.minterms[b.mCount] = a.minterms[i];
            b.mCount = b.mCount + 1;
        }
    }
}


// 두 Implicant가 병합 가능한지 검사하는 함수
bool canMerge(Implicant& a, Implicant& b){
    // 둘이 하나만 다른지를 확인하기 전에, 먼저 mask가 같은지 확인해야 함
    if (a.mask != b.mask) {
        return false;
    }

    // value의 차이가 정확히 1비트인지 확인
    // XOR 연산: 같은 값은 0, 다른 값은 1이 됨
    int diff = a.value ^ b.value;
    if (countOnes(diff) == 1) {
        return true; // 정확히 1만 다름 → 병합 가능!
    } else {
        return false; // 0 또는 2이상 다름 → 불가능
    }
}


// 두 Implicant를 병합하는 함수
Implicant doMerge(Implicant& a, Implicant& b){
    // 두 변수를 XOR 연산하여 다른 자리를 찾음
    int diff = a.value ^ b.value;

    // 병합 값을 넣어줄 구조체 생성
    Implicant merged;

    // 병합된 항의 value: diff 위치는 0으로 바뀌고 나머지는 그대로
    merged.value = a.value & (~diff);

    // 병합된 항의 mask: diff 위치는 1로 바뀌고 나머지는 그대로 (or연산)
    merged.mask = a.mask | diff;

    merged.used = false;// 새로 생성됨, 아직 병합에 안 쓰임

    // Don't Care 여부: 둘 다 DC일 때만 새 항도 DC
    if (a.isDC && b.isDC){
        merged.isDC = true;
    }
    else{
        merged.isDC = false;
    }

    // 최소항 배열 초기화 후 양쪽의 최소항들을 합침
    merged.mCount = 0;
    mergeMinterms(merged, a); // a의 minterms를 merged에 병합
    mergeMinterms(merged, b); // b의 minterms를 merged에 병합

    return merged;
}



void findPIs(){
    int curCount = 0;

    //f=1인 minterm을 먼저 등록
    for(int i = 0; i < fCount; i++){
        curList[curCount].value = fMinterms[i];
        curList[curCount].mask = 0;
        curList[curCount].used = false;
        curList[curCount].isDC = false; // f=1인 minterm은 DC가 아님
        curList[curCount].minterms[0] = fMinterms[i];
        curList[curCount].mCount = 1;
        curCount++;
    }

    // DC 등록
    for(int i = 0; i < dcCount; i++){
        curList[curCount].value = dcMinterms[i];
        curList[curCount].mask = 0;
        curList[curCount].used = false;
        curList[curCount].isDC = true; // DC minterm은 true로 표시
        curList[curCount].minterms[0] = dcMinterms[i];
        curList[curCount].mCount = 1;
        curCount++;
    }

    // 병합 시작
    while (true){
        int nextCount = 0;// 다음 라운드 리스트 크기
        bool anyMerged = false;  // 이번 라운드에서 병합이 발생했는가?

        // 현재 리스트의 모든 쌍(i, j)을 비교
        for (int i = 0; i < curCount; i++){
            for (int j = i + 1; j < curCount; j++){

                // 병합 가능한지 검사
                if (!canMerge(curList[i], curList[j])){
                    continue;  // 불가능하면 다음으로
                }

                // 병합 실행
                Implicant merged = doMerge(curList[i], curList[j]);

                // 원본 두 항은 "사용됨" 표시 (더 이상 PI 후보가 아님)
                curList[i].used = true;
                curList[j].used = true;
                anyMerged = true;

                // 다음 리스트에 이미 같은 항이 있는지 중복 검사
                bool duplicate = false;
                for (int k = 0; k < nextCount; k++){
                    if (isSame(nextList[k], merged)){
                        // 같은 항이 이미 있으면, 커버하는 최소항 목록만 합침
                        mergeMinterms(nextList[k], merged);
                        duplicate = true;
                        break;
                    }
                }

                // 중복이 아니면 다음 라운드 리스트에 새로 추가
                if (!duplicate){
                    nextList[nextCount] = merged;
                    nextCount++;
                }
            }
        }

        // --- 이번 라운드에서 병합에 사용되지 않은 항 = PI 후보 ---
        for (int i = 0; i < curCount; i++){
            if (curList[i].used) continue; // 이미 병합된 항
            if (curList[i].isDC) continue; // DC로만 이루어진 항

            bool duplicate = false;
            for (int k = 0; k < piCount; k++){
                if (isSame(PIs[k], curList[i])){
                    duplicate = true;
                    break;
                }
            }

            if(!duplicate){
                PIs[piCount] = curList[i];
                piCount++;
            }
        }

        // 이번 라운드에서 병합이 한 건도 없었으면 모든 병합이 완료됨
        if (!anyMerged) break;

        // 다음 라운드 준비: nextList를 curList로 교체
        curCount = nextCount;
        for (int i = 0; i < curCount; i++){
            curList[i] = nextList[i];
        }
    }
}



//void solve(bool rRow[MAX], bool rCol[MAX], int cost, int selCnt){}

int dominate(int a[], int b[], int length){
    int a_bitmask = 0, b_bitmask = 0; 
    for(int i = 0; i < length; i++){ //배열을 비트마스크로
        if(a[i]==1){
            a_bitmask |= (1 << i);
        }
        if(b[i]==1){
            b_bitmask |= (1 << i);
        }
    }

    if((a_bitmask & b_bitmask) == a_bitmask){ //a가 b를 포함
        return 1;
    } else if((a_bitmask & b_bitmask) == b_bitmask){ //b가 a를 포함
        return 2;
    }
    else{ return 0;} //포함하지않음
}


void minimize(){

    //coverTable 채우기
    for(int i = 0; i<piCount; i++){
        for(int j = 0; j<fCount; j++){
            coverTable[i][j]=0;
            for(int k = 0; k<PIs[i].mCount; k++){ //해당 좌표와 PI가 보유한 Minterm정보가 동일한 경우 1로 수정
                if(fMinterms[j]==PIs[i].minterms[k]){
                    coverTable[i][j]=1;
                }
            }
        }
    }

    bool flag = true;
    while(flag){
        flag = false;

        //EPI제거 
        for(int j = 0; j<fCount; j++){
            int count = 0;
            int tmp = 0;
            for(int i = 0; i<piCount; i++){ //column의 1 수를 세어 1이면 EPI
                if(coverTable[i][j]==1){
                    count++;
                    tmp = i;
                }
            }
            if(count==1){ //answer에 넣고 테이블에서 제거
                answer[ansCount] = PIs[tmp];
                ansCount++;
                for(int k = 0; k<fCount; k++){
                    coverTable[tmp][k]=0;
                }
            }
        }

        //병합가능한 row 제거
        for(int i = 0; i<piCount-1; i++){
            for(int k = i+1; k<piCount; k++){
                int domi = dominate(coverTable[i], coverTable[k], fCount);
                if(domi == 1){
                    for(int a = 0; a<fCount; a++){ //i 가 더 클경우 k 제거
                        coverTable[k][a] = 0;
                        flag = true;
                    }
                }else if(domi = 2){
                    for(int a = 0; a<fCount; a++){ //k가 더 클경우 i 제거 후 break
                        coverTable[i][a] = 0; 
                        flag = true; break;
                    }
                }
            }
        }

        //병합 가능한 column 제거
        for(int j = 0; j<fCount; j++){
            for(int k = j+1; k<fCount; k++){
                int domi = dominate(coverTable[j], coverTable[k], piCount);
                if(domi == 1){
                    for(int a = 0; a<piCount; a++){ //j 가 더 클경우 k 제거
                        coverTable[k][a] = 0;
                        flag = true;
                    }
                }else if(domi == 2){
                    for(int a = 0; a<piCount; a++){ //k가 더 클경우 j 제거 후 break
                        coverTable[j][a] = 0; 
                        flag = true; break;
                    }
                }
            }
        }
    }

    //안끝나는 경우 한쪽을선택해야됨.. 

}


int main() {
    cin >> num; //변수의 개수를 입력받음.
    cin >> fCount >> dcCount; // f=1인 minterm의 개수와 DC의 개수를 입력받음

    if(num == 0){
        printf("변수의 개수가 0입니다. 최소항이 존재할 수 없습니다.\n");
        return 0;
    }

    if(fCount == 0){
        printf("f=1인 최소항이 하나도 없습니다.\n");
        return 0;
    }
    else{
        for (int i = 0; i < fCount; i++){
            cin >> fMinterms[i]; // f=1인 minterm의 번호 입력
        }
    }
    

    if(dcCount != 0){
        for (int i = 0; i < dcCount; i++) {
                cin >> dcMinterms[i]; // DC 번호 입력
            }
    }
    
    // PI를 만드는 과정
    findPIs();


    // 구현 필요!
    // 만든 PI를 바탕으로 Finding a minimum cover
    //minimize();

    // --- 결과 출력 ---
    return 0;
}
