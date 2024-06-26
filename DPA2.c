#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned char byte;
#define NUM_LINES 1000
#define LINE_SIZE 50
#define NUM_POINTS 2700     //14864 -> 2700 으로 범위 축소 (1R SubBytes에 대한 전력만 확인)

const byte Sbox[256] = {
 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };


//Trace x point 배열(동적 할당) 반환 함수
float **Trace_array(){
    float **array;
    FILE *file = fopen("CW_Lite_powerConsumption.trace", "rb");
    if (file == NULL) {
        perror("메모리 할당 오류");
        fclose(file);
        exit(1);
    }
    fseek(file, 32, SEEK_SET); //header 32bytes
    // 행에 대한 메모리 할당
    array = (float **)malloc(NUM_LINES * sizeof(float *));
    if (array == NULL) {
        perror("메모리 할당 오류");
        fclose(file);
        free(array);
        exit(1);
    }

    // 각 행에 대한 열에 대한 메모리 할당
    for (int i = 0; i < NUM_LINES; i++) {
        array[i] = (float *)malloc(NUM_POINTS * sizeof(float));
        if (array[i] == NULL) {
            perror("메모리 할당 오류");
            for (int j = 0; j < i; j++) {
                free(array[j]);
            }
            free(array);
            fclose(file);
            exit(1);
        }
    }
    //4byte --> float
    byte buffer[4];
    //800 ~ 3000 번째 Points로 계산해본다
    for (int i = 0; i < NUM_LINES; i++) {
        fseek(file, 3200, SEEK_CUR);        //800 * 4 만큼 파일포인터 이동 800번째 Point 부터
        for (int j = 0; j < NUM_POINTS; j++) {
            if (fread(buffer, sizeof(byte), 4, file) != 4) {
                perror("파일 읽기 오류");
                for (int k = 0; k < NUM_LINES; k++) {
                    free(array[k]);
                }
                free(array);
                fclose(file);
                exit(1);
            }
            //float 값 저장
            float value = *((float *)buffer);
            array[i][j] = value;
        }
        if (i != (NUM_LINES - 1)){
            fseek(file, 45456, SEEK_CUR);   // 3000 Point 까지 읽은 후 13064 * 4 만큼 파일포인터 이동
        }
    }
    return array;
}

//plain text ==> bytearray[1000][16] 배열(정적 할당)로 변환하는 함수
void fileToArray(byte byteArray[NUM_LINES][16]) {

    FILE* fp = fopen("CW_Lite_plain.txt", "r");
    if (fp == NULL) {
        printf("파일을 열 수 없습니다.\n");
        exit(1);
    }
    char line[LINE_SIZE];
    int line_index = 0;
    while (fgets(line, sizeof(line), fp) != NULL && line_index < NUM_LINES) {
        line[strcspn(line, "\n")] = '\0';
        char *token = strtok(line, " ");
        int byte_index = 0;
        while (token != NULL && byte_index < 16) {
            byteArray[line_index][byte_index] = (byte)strtol(token, NULL, 16);
            token = strtok(NULL, " ");
            byte_index++;
        }
        if (byte_index > 0) {
            line_index++;
        }
    }
    fclose(fp);
}

//16번의 key guessing 함수
void byte_key_guess(byte byteArray[NUM_LINES][16], byte key_candidate[16]){
    byte temp;
    float mean_diff, max_diff, set_0_sum, set_1_sum;
    int sum_0, sum_1;

    float **point_array = Trace_array();  // Trace 별 포인트 

    for(int p = 0; p < 16; p ++){    //16바이트의 guess key
        max_diff = 0.0;
        for(int k = 0; k < 256; k ++){  //키 후보 00 ~ FF
            for(int pt_idx = 0; pt_idx < NUM_POINTS; pt_idx ++){
                set_0_sum = 0.0;
                set_1_sum = 0.0;
                sum_0 = 0;
                sum_1 = 0;
                for(int tr_idx = 0; tr_idx < NUM_LINES; tr_idx ++){
                    temp = Sbox[byteArray[tr_idx][p] ^ k];  //중간값 계산 --> Sbox
                    if(((temp >> 7) & 0x01) == 0){   //msb가 0이면
                        set_0_sum += point_array[tr_idx][pt_idx]; //temp 파형값 (float)
                        sum_0 += 1;
                    }
                    else{       //msb가 1이면
                        set_1_sum += point_array[tr_idx][pt_idx];  //temp 파형값 (float)
                        sum_1 += 1;
                    }
                }
                //포인트 별 |(set0평균 - set1평균)|
                mean_diff = fabsf((set_1_sum / sum_1) - (set_0_sum / sum_0));   
                if (mean_diff > max_diff){  //포인트 별 set차 중 가장 차가 큰 것
                    max_diff = mean_diff;
                    key_candidate[p] = k;   //해당바이트 guess key
                }
            }
            
        }
    }
    //point_array 할당 해제
    for (int i = 0; i < NUM_LINES; i++) {
        free(point_array[i]);
    }
    free(point_array);
}

int main(){
    byte byteArray[NUM_LINES][16];
    byte key_list[16];
    fileToArray(byteArray);
    byte_key_guess(byteArray, key_list);
    for(int i = 0; i < 16; i++){
        printf("0x%02x, ", key_list[i]);   //16바이트 master key
    }
}
