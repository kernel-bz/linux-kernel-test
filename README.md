# linux-kernel-test (kanalyzer)
Linux Kernel Source Test &amp; Analyzer

Linux Kernel 소스를 주제별로 분리하여 실행 및 분석(디버깅)할 수 있습니다.
자세한 내용은 docs 경로안에 PDF 문서 참조 바랍니다.

1. 매뉴얼
    docs/kanalyzer-quick-start-guide.pdf
    docs/kanalyzer-menu-guide.pdf

qt5와 urcu 패키지가 설치 되어 있어야 합니다.

2. qt5 설치
$ sudo apt-get install build-essential qtcreator qt5-default qt5-doc

3. userspace RCU 설치
$ sudo apt-get install liburcu-dev

4. qt project 파일 열기
kernel-test.pro

기타 궁금한 사항은 커널연구회로 메일(rgbi3307@nate.com) 주시기 바랍니다.
감사합니다.
