# linux-kernel-test (kanalyzer)
Linux Kernel Source Test &amp; Analyzer

Linux Kernel 소스를 user application(qt project)에서 실행 및 분석(디버깅)할 수 있습니다.

자세한 내용은 docs 경로안에 PDF 문서 참조 바랍니다.


1. 매뉴얼

    docs/kanalyzer-quick-start-guide.pdf
    
    docs/kanalyzer-menu-guide.pdf


qt와 urcu 패키지가 설치 되어 있어야 합니다.


2. qt 설치

    우분투 배포본 버전 18.04에서 설치:
    
    $ sudo apt-get install build-essential qtcreator qt5-default qt5-doc
    
    우분투 배포본 버전 22.04에서 설치:
    
    $ sudo apt-get install build-essential qtcreator qtbase5-dev qt5-qmake cmake qt6-base-dev


3. userspace RCU 설치

    $ sudo apt-get install liburcu-dev


4. qt project 파일 열기

    kernel-test.pro


5. 출판 서적(소스 설명)

    소스에 대한 자세한 설명은 아래의 출판 서적을 참고 하시기 바랍니다.
    
    책제목: "리눅스 커널 소스 해설2 [소스 실행 분석기]"    
    
    커널연구회: https://www.kernel.bz/product/Books-15
        
기타 궁금한 사항은 커널연구회로 메일(rgbi3307@nate.com) 주시기 바랍니다.

감사합니다.
