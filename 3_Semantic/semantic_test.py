import os
import sys
import subprocess

def main():
    success_cases = os.listdir('./test_case_s')
    failure_cases = os.listdir('./test_case_e')

    for case in success_cases:
        proc = subprocess.run(['./cminus_semantic', './test_case_s/' + str(case)], capture_output=True)
        if 'error' in proc.stdout.decode('utf-8'):
            print(f'테스트 실패: \n{proc.stdout.decode("utf-8")}')
        else:
            print(f"테스트 성공: {case}")
    
    
    for case in failure_cases:
        proc = subprocess.run(['./cminus_semantic', './test_case_e/' + str(case)], capture_output=True)
        if 'error' not in proc.stdout.decode('utf-8'):
            print(f'테스트 실패: \n{proc.stdout.decode("utf-8")}')
        else:
            s = '\n'.join([i for i in proc.stdout.decode('utf-8').split('\n') if 'error' in i])
            print(f"테스트 성공: {case}\n  {s}")

if __name__ == '__main__':
    main()