#include "../run2.h"
void grad_case4(float (&B)[16][32], float (&C)[32][32], float (&dA)[16][32], float (&dB)[16][32], float (&dC)[32][32]) {
  for (int i = 0;i < 16; ++i){
    for (int j = 0;j < 32; ++j){
      for (int k = 0;k < 32; ++k){
        dB[i][k] +=  ( dA[i][j] * C[k][j] ) ;
        dC[k][j] +=  ( B[i][k] * dA[i][j] ) ;
      }
    }
  }
}
