import os
import re
import subprocess

def main():
    success_cases = os.listdir('./test_case_s')
    failure_cases = os.listdir('./test_case_e')
    print(success_cases)

    for case in sorted(success_cases):
        proc = subprocess.run(['./cminus_semantic', './test_case_s/' + str(case)], capture_output=True)
        r = proc.stdout.decode('utf-8')
        if 'error' in r:
            print(f'테스트 실패: {case} \n{r}')
        else:
            print(f"테스트 성공: {case}")
    
    
    for case in sorted(failure_cases):
        proc = subprocess.run(['./cminus_semantic', './test_case_e/' + str(case)], capture_output=True)
        with open('./test_case_e/' + str(case), 'r') as f:
            try:
                a = [int(re.sub(r'[^0-9]', '', i)) for i in f.readlines() if '/* error at' in i]
                if a:
                    error_line_check = a[0]
                else:
                    error_line_check = None
            except:
                pass

        r = proc.stdout.decode('utf-8')
        if 'error' not in r:
            print(f'테스트 실패: {case} \n{r}\n')
        else:
            s = '\n'.join([i for i in r.split('\n') if 'error' in i])
            if error_line_check:
                error_line = int(re.sub(r'[^0-9]', '', s.split(':')[0]))
                if error_line_check != error_line:
                    print(f'테스트 실패(at line 다름): {case} {error_line_check} != {error_line}\n')
            
            print(f"테스트 성공: {case}\n  {s}\n")

if __name__ == '__main__':
    main()