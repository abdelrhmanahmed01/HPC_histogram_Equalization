#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <mpi.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) 
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

float arr2[256] = { 0 };
float copy_arr[256];
float final[256];


int set_num_of_pixels(int arr[], int size)
{


	for (int i = 0; i < size; i++)
	{
		arr2[arr[i]]++;
		//copy_arr[arr[i]]++;
	}

	return 0;
}
void get_com_prob(int size)
{
	float cnt = 0;
	for (int i = 0; i < 256; i++)
	{
		arr2[i] = arr2[i] / size;
		if (i > 0) {
			arr2[i] = arr2[i] + arr2[i - 1];
			//copy_arr[i] = arr2[i];
			//cnt += arr2[i];
		}
	}
}
void new_pixel(int image[], int x)
{
	for (int i = 0; i < 256; i++)
	{
		arr2[i] = floor(arr2[i] * 255);

	}
	for (int i = 1; i < x; i++)
	{
		for (int j = 1; j <= 256; j++)
		{
			if (image[i] == j)
			{
				//cout << image[i] << endl;
				image[i] = arr2[j];
				//cout << image[i] << endl;
			}
		}
	}

}

int main()
{
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0;

	System::String^ imagePath;
	std::string img;
	img = "..//10N.png";

	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);

	start_s = clock();
	MPI_Init(NULL, NULL);
	int numPixels = ImageHeight * ImageWidth;
	int size, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int size_of_every_procss = numPixels / size;
	int last_pixels = numPixels % size;
	float* sum = NULL;
	int arr[256] = {0};
	int* size_of_every_procss_arr = (int*)malloc(sizeof(int) * size_of_every_procss);
	int* Image = NULL;
	//set_num_of_pixels(imageData, numPixels);
	//get_com_prob(numPixels);
	//new_pixel(imageData, numPixels);
	if (rank == 0) {
		Image = imageData;
	}
	
	MPI_Scatter(Image, size_of_every_procss, MPI_INT, size_of_every_procss_arr, size_of_every_procss, MPI_INT, 0, MPI_COMM_WORLD);
	
	//int x=0;
	//while (localpart[x] != NULL) {
		//arr[localpart[x]]++;
		//x++;

	//}
	for (int i = 0; i < size_of_every_procss; i++) {
       arr[size_of_every_procss_arr[i]]++;
	}
	
	if (rank == 0)
		MPI_Reduce(MPI_IN_PLACE, arr, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	else
		MPI_Reduce(arr, nullptr, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	
	if (rank == 0 && last_pixels != 0) {
		for (int i = (size_of_every_procss * size); i < numPixels; i++) {
			arr[Image[i]]++;
		}
	}
	//Calculating the Cummulative Probability 
	float* Prob = NULL;
	if (rank == 0) {
		Prob = new float[256];
		for (int i = 0; i < 256; i++) {
			Prob[i] = (double)arr[i] / numPixels;
		}
	}
	if (rank == 0) {
		for (int i = 1; i < 256; i++) {
			Prob[i] = Prob[i] + Prob[i - 1];
		}
	}
	if (rank == 0) {
		for (int i = 0; i < 256; i++) {
			arr[i] = Prob[i] * 255;
		}
	}
	

	MPI_Bcast(arr, 256, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Status stat;
	
	if (rank == 0) {
		for (int i = 1; i < size; i++) {
			MPI_Send(Image + (i * size_of_every_procss), size_of_every_procss, MPI_INT, i, i, MPI_COMM_WORLD);
		}
		for (int i = 0; i < size_of_every_procss; i++) {
			Image[i] = arr[Image[i]];
		}
		for (int i = (size * size_of_every_procss); i < numPixels; i++) {
			Image[i] = arr[Image[i]];
		}
		for (int i = 1; i < size; i++) {
			MPI_Recv(Image + (i * size_of_every_procss), size_of_every_procss, MPI_INT, i, i, MPI_COMM_WORLD, &stat);
		}
	}
	else {
		MPI_Recv(size_of_every_procss_arr, size_of_every_procss, MPI_INT, 0, rank, MPI_COMM_WORLD, &stat);
		for (int i = 0; i < size_of_every_procss; i++) {
			size_of_every_procss_arr[i] = arr[size_of_every_procss_arr[i]];
		}
		MPI_Send(size_of_every_procss_arr, size_of_every_procss, MPI_INT, 0, rank, MPI_COMM_WORLD);
	}
	MPI_Finalize();
	
	if (rank == 0) {
		createImage(Image, ImageWidth, ImageHeight, rank);
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << TotalTime << endl;
	}
	free(imageData);
	return 0;

}
