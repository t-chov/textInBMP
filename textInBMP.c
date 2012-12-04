#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>

#define BUFSIZE 800000
enum COLOR { BLACK, WHITE };

typedef struct tagBITMAPFILEHEADER {
	unsigned short bfType;
	unsigned int   bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	unsigned int   biSize;
	int            biWidth;
	int            biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int   biCompression;
	unsigned int   biSizeImage;
	int            biXPixPerMeter;
	int            biYPixPerMeter;
	unsigned int   biClrUsed;
	unsigned int   biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD{
	unsigned char rgbBlue;
	unsigned char rgbGreen;
	unsigned char rgbRed;
	unsigned char rgbReserved;
} RGBQUAD;

//ファイルヘッダの敷設
void setFileHeader(BITMAPFILEHEADER *fHeader, unsigned int size){
	fHeader->bfType	= htons(0x424d);
	fHeader->bfSize	= size;
	fHeader->bfReserved1 = 0x0000;
	fHeader->bfReserved2 = 0x0000;
	//2色BMPなので
	//fHeader->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 2);
	fHeader->bfOffBits = 0x0000003E;
	printf("HEADER:\n\tSize: 0x%08x\n\tOfst: 0x%08x\n",
					fHeader->bfSize, fHeader->bfOffBits);
}
//ヘッダの書き出し
void writeFileHeader(BITMAPFILEHEADER *fHeader, 
										 BITMAPINFOHEADER *iHeader, FILE *fp){
	fwrite(&(fHeader->bfType),      sizeof(short), 1, fp);
	fwrite(&(fHeader->bfSize),      sizeof(int),   1, fp);
	fwrite(&(fHeader->bfReserved1), sizeof(short), 1, fp);
	fwrite(&(fHeader->bfReserved2), sizeof(short), 1, fp);
	fwrite(&(fHeader->bfOffBits),   sizeof(int),   1, fp);

	fwrite(&(iHeader->biSize),        sizeof(int),   1, fp);
	fwrite(&(iHeader->biWidth),       sizeof(int),   1, fp);
	fwrite(&(iHeader->biHeight),      sizeof(int),   1, fp);
	fwrite(&(iHeader->biPlanes),      sizeof(short), 1, fp);
	fwrite(&(iHeader->biBitCount),    sizeof(short), 1, fp);
	fwrite(&(iHeader->biCompression), sizeof(int),   1, fp);
	fwrite(&(iHeader->biSizeImage),   sizeof(int),   1, fp);
	fwrite(&(iHeader->biXPixPerMeter),sizeof(int),   1, fp);
	fwrite(&(iHeader->biYPixPerMeter),sizeof(int),   1, fp);
	fwrite(&(iHeader->biClrUsed),     sizeof(int),   1, fp);
	fwrite(&(iHeader->biClrImportant),sizeof(int),   1, fp);
}

//ファイルサイズの取得
unsigned int getFileSize(FILE *fp){
	int nowPoint = ftell(fp);
	fseek(fp, 0, SEEK_END);
	unsigned int fSize = (unsigned int)ftell(fp);
	fseek(fp, nowPoint, SEEK_SET);
	return fSize;
}
//情報ヘッダの敷設
void setCoreHeader(BITMAPINFOHEADER *cHeader, int height){
	cHeader->biSize     = 40;
	cHeader->biWidth    = 320; //widthは320(40文字)固定
	cHeader->biHeight   = height;
	cHeader->biPlanes   = 1;
	cHeader->biBitCount = 1;
	//ここから基本的に変わらない
	cHeader->biCompression = 0;
	cHeader->biSizeImage = 3780;
	cHeader->biXPixPerMeter = 3780;
	cHeader->biYPixPerMeter = 3780;
	cHeader->biClrUsed = 2;
	cHeader->biClrImportant = 0;
}
//メッセージの埋め込み
void writeMessage(FILE *fp, char *message){
	fseek(fp, 62, SEEK_SET);
	fwrite(message, sizeof(char), strlen(message), fp);
	fwrite(message, sizeof(char), strlen(message), stdout);
	printf("EOF\n");
}
//ファイルからメッセージを読み取り
//バッファはBUFSIZEまで
void getMessage(char *filename, char *messagebuf){
	FILE *fp;
	if((fp = fopen(filename, "rb")) == NULL){
		printf("cannot open the file: %s", filename);
		return;
	}

	unsigned int fSize = getFileSize(fp);
	if(fSize < BUFSIZE){
		fread(messagebuf, (size_t)fSize, (size_t)(fSize / sizeof(char)), fp);
		return;
	}else{
		printf("this file is too large to write");
		return;
	}
}
//messageを40文字区切りにする
void formatMessage(char *message){
	int padding = 0, i = 0, len=strlen(message);
	if((len%40) == 0){
		return;
	}else{
		padding = (floor(len / 40)+1) * 40 - len;
		for(i; i < padding; i++){
			message[len + i] = 0x20;
		}
	}
	return;
}
//ファイル作成前にbmpファイルの大きさを決める
//メッセージを確保してから呼ぶ
unsigned int getBMPFileSize(char *message){
	return (unsigned int)(62 + strlen(message));
}

int main(int argc, char **argv){
	//メッセージ
	char message[BUFSIZE] = {0};
	if(argc == 1){
		printf("Message: ");
		fgets(message, BUFSIZE, stdin);
	}else{
		getMessage(argv[1], message);
	}
	//メッセージをフォーマット
	formatMessage(message);
	//ファイルサイズ
	unsigned int outputFileSize = 0x00000000 + getBMPFileSize(message);
	//新規ファイルを作成
	FILE *fp;
	fp = fopen("output.bmp", "wb");

	if(fp == NULL){
		printf("ERROR\n");
		return -1;
	}

	//ヘッダの設定
	BITMAPFILEHEADER fHeader;
	memset(&fHeader, 0x00, 14);
	setFileHeader(&fHeader, outputFileSize);
	//fwrite(&fHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	BITMAPINFOHEADER cHeader;
	memset(&cHeader, 0x00, 40);
	setCoreHeader(&cHeader, (int)strlen(message) / 40);
	//fwrite(&cHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	writeFileHeader(&fHeader, &cHeader, fp);
	//パレットの設定
	RGBQUAD rgb[2];
	//クロ
	rgb[BLACK].rgbBlue  = 0;
	rgb[BLACK].rgbGreen = 0;
	rgb[BLACK].rgbRed   = 0;
	rgb[BLACK].rgbReserved = 0;
	//白
	rgb[WHITE].rgbBlue  = 0xff;
	rgb[WHITE].rgbGreen = 0xff;
	rgb[WHITE].rgbRed   = 0xff;
	rgb[WHITE].rgbReserved = 0;
	fwrite(rgb, sizeof(RGBQUAD)*2, 2, fp);
	
	writeMessage(fp, message);
	fclose(fp);

return 0;
}

