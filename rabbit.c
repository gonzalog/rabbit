#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define WORDSIZE 0x100000000

struct bit {
  unsigned int value : 1;
};

int64_t square(int64_t n){
  return (n % WORDSIZE) * (n % WORDSIZE);
}

int16_t lsw(int32_t n){
  return n % 0x00010000;
}

int16_t msw(int32_t n){
  return n / 0x00010000;
}

int32_t g(u, v){
  return lsw(square(u+v)) ^ msw(square(u+v));
}

char* next_state(int32_t* X, int32_t* C, struct bit* counter_carry){
  int16_t* S = malloc(sizeof(int16_t) * 8);
  int32_t* G = malloc(sizeof(int32_t) * 8);
  int32_t* A = malloc(sizeof(int32_t) * 8);

  A[0] = 0x4D34D34D;
  A[1] = 0xD34D34D3;
  A[2] = 0x34D34D34;
  A[3] = 0x4D34D34D;
  A[4] = 0xD34D34D3;
  A[5] = 0x34D34D34;
  A[6] = 0x4D34D34D;
  A[7] = 0xD34D34D3;

  int j, i, g_iterator;

  for(j = 0; j < 4; j++){
    // Counter Update
    for(i = 0; i < 8; i++){
      (*counter_carry).value = (C[i] + A[i] + (*counter_carry).value) / WORDSIZE;
      C[i] = (C[i] + A[i] + (*counter_carry).value) % WORDSIZE;
    }
    // Next State Function
    for(g_iterator = 0; g_iterator < 8; g_iterator++)
      G[g_iterator] = g(X[g_iterator],C[g_iterator]);

    X[0] = G[0] + (G[7] << 16) + (G[6] << 16) % WORDSIZE;
    X[1] = G[1] + (G[0] <<  8) +  G[7] % WORDSIZE;
    X[2] = G[2] + (G[1] << 16) + (G[0] << 16) % WORDSIZE;
    X[3] = G[3] + (G[2] <<  8) +  G[1] % WORDSIZE;
    X[4] = G[4] + (G[3] << 16) + (G[2] << 16) % WORDSIZE;
    X[5] = G[5] + (G[4] <<  8) +  G[3] % WORDSIZE;
    X[6] = G[6] + (G[5] << 16) + (G[4] << 16) % WORDSIZE;
    X[7] = G[7] + (G[6] <<  8) +  G[5] % WORDSIZE;

    // Output
    S[7] = lsw(X[0]) ^ msw(X[5]);
    S[6] = msw(X[0]) ^ lsw(X[3]);
    S[5] = lsw(X[2]) ^ msw(X[7]);
    S[4] = msw(X[2]) ^ lsw(X[5]);
    S[3] = lsw(X[4]) ^ msw(X[1]);
    S[2] = msw(X[4]) ^ lsw(X[7]);
    S[1] = lsw(X[6]) ^ msw(X[3]);
    S[0] = msw(X[6]) ^ lsw(X[1]);
  }

  return (char*) S;
}

void key_setup(int16_t* K, int32_t* X, int32_t* C){
  int j;
  for(j = 0; j <= 6 ; j++){
    X[j] = (int32_t) K[(j+1) % 8] << 16 | K[j];
    C[j] = (int32_t) K[(j+4) % 8] << 16 | K[(j+5) % 8];
    
    j++;
    X[j] = (int32_t) K[(j+5) % 8] << 16 | K[(j+4) % 8];
    C[j] = (int32_t) K[j] << 16 | K[(j+1) % 8];
  }  
}

void rabbit(FILE* file, int16_t* K, FILE* output){
  // Inner State
  int32_t* X = malloc(sizeof(int32_t) * 8);
  int32_t* C = malloc(sizeof(int32_t) * 8);
  struct bit counter_carry;

  // Counter Carry Setup
  counter_carry.value = 0;

  //Key Setup
  key_setup(K, X, C);

  // 4 Next state iterations to remove linearity on key
  int i;
  for(i = 0; i < 4; i ++)
    next_state(X, C, &counter_carry);

  // Reinit Counters
  for(i = 0; i < 8; i++)
    C[i] = C[i] ^ X[(i+4) % 8];

  // Initial Value Setup

  // 4 Next state iterations to remove linearity on iv
  // for(i = 0; i < 4; i ++)
  //   next_state(X, C, &counter_carry);

  // Iteration over file
  char* encrypt_block;
  int block_iterator;
  char* buffer = malloc(sizeof(char) * 16);
  
  for(i = 0; !feof(file); i++){
    fread(&buffer[i], 1, 1, file);

    if(i == 15){
      encrypt_block = next_state(X, C, &counter_carry);

      for(block_iterator = 0; block_iterator < 16; block_iterator++){
        buffer[block_iterator] = buffer[block_iterator] ^ encrypt_block[block_iterator];
      }

      fwrite(buffer, 1, 16, output);
      
      i = 0;
    }
  }

  if(i > 0){
    encrypt_block = next_state(X, C, &counter_carry);
    
    for(block_iterator = 15; block_iterator > (15 - i); block_iterator--){
      buffer[i - 1] = buffer[i - 1] ^ encrypt_block[16 - i];
    }
    
    fwrite(buffer, 1, i, output);
    return;
  }
}

// First param is a key of 16 chars and a message.
int main(int argc, char** argv){
  int16_t* K = (int16_t*) argv[1];
  FILE* file = fopen(argv[2], "rb");
  FILE* output = fopen(argv[3], "wb");

  rabbit(file, K, output);
  return 1;
}
