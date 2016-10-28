#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define WORDSIZE 0x100000000

int header_size;

struct bit {
  unsigned int value : 1;
};

int32_t concat_words(int16_t wordA, int16_t wordB){
  int16_t* result = malloc(sizeof(int16_t) * 2);
  result[0] = wordA;
  result[1] = wordB;
  return *((int32_t*) result);
}

int64_t square(int64_t n){
  return (n % WORDSIZE) * (n % WORDSIZE);
}

int16_t lsw(int32_t n){
  return ((int16_t*) &n)[1];
}

int16_t msw(int32_t n){
  return ((int16_t*) &n)[0];
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


  // Counter Update
  int i;
  for(i = 0; i < 8; i++){
    (*counter_carry).value = (C[i] + A[i] + (*counter_carry).value) / WORDSIZE;
    C[i] = (C[i] + A[i] + (*counter_carry).value) % WORDSIZE;
  }

  for(i = 0; i < 8; i++)
    G[i] = g(X[i],C[i]);

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

  return (char*) S;
}

void key_setup(int16_t* K, int32_t* X, int32_t* C){
  int j;
  for(j = 0; j <= 6 ; j++){
    X[j] = concat_words(K[(j+1) % 8] << 16, K[j]);
    C[j] = concat_words(K[(j+4) % 8] << 16, K[(j+5) % 8]);
    
    j++;
    X[j] = concat_words(K[(j+5) % 8] << 16, K[(j+4) % 8]);
    C[j] = concat_words(K[j] << 16, K[(j+1) % 8]);
  }  
}

void copy_header(FILE* file, FILE* output){
  char* header = malloc(header_size);
  fread(header, header_size, 1, file);
  fwrite(header, header_size, 1, output);
  free(header);
}

void rabbit(int32_t* X, int32_t* C, struct bit counter_carry, FILE* file, FILE* output){
  char* encrypt_block;
  int block_iterator, buffer_length;
  char* buffer = malloc(sizeof(char) * 16);
  
  copy_header(file, output);

  buffer_length = 0;
  while(!feof(file)){
    fread(&buffer[buffer_length], 1, 1, file);

    if(buffer_length == 15){
      encrypt_block = next_state(X, C, &counter_carry);

      for(block_iterator = 0; block_iterator < 16; block_iterator++){
        buffer[block_iterator] = buffer[block_iterator] ^ encrypt_block[block_iterator];
      }

      fwrite(buffer, 16, 1, output);
      
      buffer_length = 0;
    } else if(!feof(file)){
      buffer_length++;
    }
  }

  if(buffer_length > 0){
    encrypt_block = next_state(X, C, &counter_carry);
    
    for(block_iterator = buffer_length - 1; block_iterator >= 0; block_iterator--){
      buffer[block_iterator] = buffer[block_iterator] ^ encrypt_block[15 - block_iterator];
    }
    
    fwrite(buffer, buffer_length, 1, output);
    return;
  }
}

void avoid_linearity(int32_t* X, int32_t* C, struct bit* counter_carry){
  int i;
  for(i = 0; i < 4; i++)
    next_state(X, C, counter_carry);
}

void reinit_counters(int32_t* X, int32_t* C){
  int i;
  for(i = 0; i < 8; i++)
    C[i] = C[i] ^ X[(i+4) % 8];
}

void initial_value_setup(int32_t* C, int16_t* initial_value){
  // Initial Value Setup
  C[0] = C[0] ^ concat_words(initial_value[2], initial_value[3]);
  C[1] = C[1] ^ concat_words(initial_value[0], initial_value[2]);
  C[2] = C[2] ^ concat_words(initial_value[0], initial_value[1]);
  C[3] = C[3] ^ concat_words(initial_value[1], initial_value[3]);
  C[4] = C[4] ^ concat_words(initial_value[2], initial_value[3]);
  C[5] = C[5] ^ concat_words(initial_value[0], initial_value[2]);
  C[6] = C[6] ^ concat_words(initial_value[0], initial_value[1]);
  C[7] = C[7] ^ concat_words(initial_value[1], initial_value[3]);
}

// First param is a key of 16 chars and a message.
int main(int argc, char** argv){
  int16_t* K = (int16_t*) argv[1];
  FILE* file = fopen(argv[2], "rb");
  FILE* output = fopen(argv[3], "wb");
  header_size = atoi(argv[4]);

  // Inner State Declaration
  int32_t* X = malloc(sizeof(int32_t) * 8);
  int32_t* C = malloc(sizeof(int32_t) * 8);
  struct bit counter_carry;
  counter_carry.value = 0;

  key_setup(K, X, C);
  avoid_linearity(X, C, &counter_carry);

  reinit_counters(X, C);

  if(argc > 5){
    initial_value_setup(C, (int16_t*) argv[5]);
    avoid_linearity(X, C, &counter_carry);
  }

  rabbit(X, C, counter_carry, file, output);
  return 1;
}
