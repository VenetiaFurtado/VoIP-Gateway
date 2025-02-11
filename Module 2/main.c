/**
 * @file main.c
 * @author James Way | Venetia Furtado
 * @brief ECEN 5803 Mastering Embedded System Architecture
 * @brief University of Colorado, Boulder
 * @brief Project 2 Module 2
 * @brief The file contains the implementation of the mu-law algorithm for the
 * G.711 coder/decoder. The functions MuLaw_Encode() and MuLaw_Decode() were
 * referenced from an online resources listed below in the References section.
 * @version 0.1
 * @date 2024-11-09
 * @copyright Copyright (c) 2024
 * 
 * References:
 * https://dystopiancode.blogspot.com/2012/02/pcm-law-and-u-law-companding-algorithms.html
 * https://www.cs.columbia.edu/~hgs/research/projects/NetworkAudioLibrary/nal_spring/src/Codecs/g711.cpp
 * http://soundfile.sapp.org/doc/WaveFormat/
 * https://www.recordingblogs.com/wiki/format-chunk-of-a-wave-file
 * 
 */
#include <stdio.h>
#include <stdint.h>

#define PCM_HEADER_SIZE 44

//RIFF header for ITU G.711 u-law
uint8_t encodeHeader[] = {
   'R', 'I', 'F', 'F', // ChunkID
   0x00, 0x00, 0x00, 0x00, // ChunkSize
   'W', 'A', 'V', 'E', // Format
   'f', 'm', 't', 0x20, // Subchunk1ID
   0x10, 0x00, 0x00, 0x00, // Subchunk1Size
   0x07, 0x00, // Audio Format = ITU G.711 u-law
   0x01, 0x00, // NumChannels
   0x80, 0x3E, 0x00, 0x00, // Sample Rate 8000
   0x80, 0x3E, 0x00, 0x00, // Byte Rate
   0x01, 0x00, // Block Align
   0x08, 0x00, // BitsPerSample
   'd', 'a', 't', 'a', // SubChunk2ID
   0x00, 0x00, 0x00, 0x00 // SubChunk2Size
};

//RIFF header for PCM
uint8_t decodeHeader[] = {
   'R', 'I', 'F', 'F', // ChunkID
   0x00, 0x00, 0x00, 0x00, // ChunkSize
   'W', 'A', 'V', 'E', // Format
   'f', 'm', 't', 0x20, // Subchunk1ID
   0x10, 0x00, 0x00, 0x00, // Subchunk1Size
   0x01, 0x00, // Audio Format = PCM
   0x01, 0x00, // NumChannels
   0x40, 0x1F, 0x00, 0x00, // Sample Rate 8000
   0x80, 0x3E, 0x00, 0x00, // Byte Rate 16000
   0x02, 0x00, // Block Align
   0x10, 0x00, // BitsPerSample
   'd', 'a', 't', 'a', // SubChunk2ID
   0x00, 0x00, 0x00, 0x00 // SubChunk2Size
};

/**
 * @brief µ-Law Compression (Encoding) Algorithm
 * Reference: https://dystopiancode.blogspot.com/2012/02/pcm-law-and-u-law-companding-algorithms.html
 * @param number 
 * @return int8_t 
 */
int8_t MuLaw_Encode(int16_t number)
{
   const uint16_t MULAW_MAX = 0x1FFF;
   const uint16_t MULAW_BIAS = 33;
   uint16_t mask = 0x1000;
   uint8_t sign = 0;
   uint8_t position = 12;
   uint8_t lsb = 0;
   if (number < 0)
   {
      number = -number;
      sign = 0x80;
   }
   number += MULAW_BIAS;
   if (number > MULAW_MAX)
   {
      number = MULAW_MAX;
   }
   for (; ((number & mask) != mask && position >= 5); mask >>= 1, position--)
      ;
   lsb = (number >> (position - 4)) & 0x0f;
   return (~(sign | ((position - 5) << 4) | lsb));
}

/**
 * @brief µ-Law Expanding (Decoding) Algorithm
 * Reference: https://dystopiancode.blogspot.com/2012/02/pcm-law-and-u-law-companding-algorithms.html
 * @param number 
 * @return int16_t 
 */
int16_t MuLaw_Decode(int8_t number)
{
   const uint16_t MULAW_BIAS = 33;
   uint8_t sign = 0, position = 0;
   int16_t decoded = 0;
   number = ~number;
   if (number & 0x80)
   {
      number &= ~(1 << 7);
      sign = -1;
   }
   position = ((number & 0xF0) >> 4) + 5;
   decoded = ((1 << position) | ((number & 0x0F) << (position - 4)) | (1 << (position - 5))) - MULAW_BIAS;
   return (sign == 0) ? (decoded) : (-(decoded));
}

/**
 * @brief Function to open a file with required permissions (read "rb" or write "w")
 * @param filename 
 * @param permissions 
 * @return FILE* 
 */
FILE* openFile(const char* filename, const char* permissions)
{
   FILE *file;
   
   file = fopen(filename, permissions);
   if (file == NULL)
   {
      perror("Error opening file");
      return NULL;
   }
   return file;
}

/**
 * @brief Function to encode a file from 16-bit PCM format to 8-bit ITU G.711
 * 
 * @param filename 
 * @return int 
 */
int encodeFile(const char* filename)
{
   int16_t inputData;                        // Stores each byte read from the file
   //Open the file in binary read mode ("rb")
   FILE *inputFile = openFile(filename, "rb");
   if(inputFile == NULL)
   {
      return 0;
   }

   //Open the ouput file in write mode ("w")
   FILE *outputFile = openFile("encode.wav","w");
   if(outputFile == NULL)
   {
      return 0;
   }

   fseek(inputFile, PCM_HEADER_SIZE, SEEK_SET);

   fwrite(&encodeHeader, sizeof(encodeHeader), 1, outputFile);

   uint32_t count = 0;
   // Read 16-bit at a time until end of file (EOF)
   while (fread(&inputData, 2, 1, inputFile) == 1)
   {
      int8_t outputData = MuLaw_Encode(inputData);
      fwrite(&outputData, 1, 1, outputFile);
      count++;
   }

   // Move to 40 bytes from the start of the file(Subchunk2Size)
   fseek(outputFile, 40, SEEK_SET);
   fwrite(&count, 4, 1,outputFile);

   // Move to 4 bytes from the start of the file(ChunkSize)
   fseek(outputFile, 4, SEEK_SET);
   count = count + 36; //36 + Subchunk2Size
   fwrite(&count, 4, 1,outputFile);

   // Close the file
   fclose(inputFile);
   fclose(outputFile);
   return 0;
}

/**
 * @brief Function to encode a file from 8-bit ITU G.711 to 16-bit PCM format
 * 
 * @param filename 
 * @return int 
 */
int decodeFile(const char* filename)
{
   int8_t inputData;                        // Stores each byte read from the file
   //Open the file in binary read mode ("rb")
   FILE *inputFile = openFile(filename, "rb");
   if(inputFile == NULL)
   {
      return 0;
   }

   //Open the ouput file in write mode ("w")
   FILE *outputFile = openFile("decode.wav","w");
   if(outputFile == NULL)
   {
      return 0;
   }

   fseek(inputFile, PCM_HEADER_SIZE + 12, SEEK_SET);

   fwrite(&decodeHeader, sizeof(decodeHeader), 1, outputFile);
 
   uint32_t count = 0;
   // Read 8-bit at a time until end of file (EOF)
   while (fread(&inputData, 1, 1, inputFile) == 1)
   {
      int16_t outputData = MuLaw_Decode(inputData);
      fwrite(&outputData, 2, 1, outputFile);
      count += 2; 
   }

   // Move to 40 bytes from the start of the file(Subchunk2Size)
   fseek(outputFile, 40, SEEK_SET);
   fwrite(&count, 4, 1,outputFile);

   // Move to 4 bytes from the start of the file(ChunkSize)
   fseek(outputFile, 4, SEEK_SET);
   count = count + 36; //36 + Subchunk2Size
   fwrite(&count, 4, 1,outputFile);

   // Close the file
   fclose(inputFile);
   fclose(outputFile);
   return 0;
}

/**
 * @brief Functions prints data of file-used for debugging
 * 
 * @param filename 
 */
void printData(const char *filename)
{
   //Open the  file
   FILE *file = openFile(filename, "rb");
   if (file == NULL)
   {
      return;
   }

   // Read 8-bit at a time
   uint8_t byte;
   int count =0;
   while (fread(&byte, 1, 1, file) == 1)
   {
      printf("%02X ", byte); // Print each byte in hexadecimal format
      count++;
      if (count == 100)
      {
         break;
      }
   }
}

/**
 * @brief Main function consists of calls to enocde and decode the files.
 * 
 * @return int 
 */
int main()
{
   encodeFile("1_A_eng_m1.wav");
   decodeFile("3_1449183537-A_eng_m1.wav");
   //printData("3_1449183537-A_eng_m1.wav");
   return 0;
}
