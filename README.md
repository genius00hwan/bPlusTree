# bPlusTree

# Class BPlusTree 
- ReadMetadata(): 파일의 헤더에 저장된 메타데이터
- getPath() : 넘겨준 key 값에 해당하는 노드의 위치와 경로 반환 
- insert() : B+tree에 엔트리 삽입
    - insertEmpty() : binary 파일에 헤더 정보 말고 아무 노드도 없는 경우
    - insertInstoOnlyOne() : 파일에 노드 하나만 있는 경우
    - insertLeaf() : leaf노드에 data 삽입
    - propagate() : 데이터가 삽입됐을 때 부모 노드까지 propagate.
- search() : data하나를 getPath를 통해 위치를 이용해 검색
- rangeSearch() : data하나를 getPath를 통해 위치를 이용해 검색 범위에 해당하는 결과를 vector를 이용해 반환
- print() : depth 0, 1인 노드 출력

## How to compile and run
-    cpu : macM1, OS : macOS
-    Language : C++14, IDE : CLion

