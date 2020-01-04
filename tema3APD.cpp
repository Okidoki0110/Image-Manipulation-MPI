#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <mpi.h>

#include "filters.h"


const char * usage = "mpirun -np N ./tema3 image_in.pnm image_out.pnm filter1 filter2 ... filterX";


int main(int argc, char ** argv) {
  std::cout << std::unitbuf;
  MPI_Init( & argc, & argv);

  int NThreads;
  MPI_Comm_size(MPI_COMM_WORLD, & NThreads);

  int thread_id;
  MPI_Comm_rank(MPI_COMM_WORLD, & thread_id);

  if (argc < 4) {
    std::cout << "Usage: " << usage << std::endl;
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  if (thread_id == 0) {
    std::ifstream input_image;
    input_image.open(argv[1], std::ios::binary);
    if (!input_image.is_open()) {
      std::cerr << "Unable to open input image!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    input_image.seekg(0, input_image.end);
    size_t input_image_size = input_image.tellg();
    input_image.seekg(0, input_image.beg);

    char * image_file_buffer = new(std::nothrow) char[input_image_size];
    if (image_file_buffer == NULL) {
      std::cerr << "Unable to allocate memory to hold the input image!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    input_image.read(image_file_buffer, input_image_size);
    input_image.close();

    bool image_bw = image_file_buffer[1] == '5';

    char res_x_ascii[5], res_y_ascii[5];
    memset(res_x_ascii, 0, 5);
    memset(res_y_ascii, 0, 5);

    char * first_header_row = strstr(image_file_buffer, "\n");
    if (first_header_row == NULL) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    first_header_row++;

    /*
    Aici apare o problema cu formatele PGM/PNM nedocumentata in cerinta.
    Aparent structura (headerul) PGM/PNM poate contine comentarii.
    Un rand de comentarii incepe cu #
    */
    while (first_header_row[0] == '#') {
      first_header_row = strstr(first_header_row, "\n");

      if (first_header_row == NULL) {
        std::cerr << "Error parsing input image header!" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, -1);
      }

      first_header_row++;
    }

    char * res_header_separator = strstr(first_header_row, " ");
    if (res_header_separator == NULL) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    memcpy(res_x_ascii, first_header_row, (res_header_separator - first_header_row));
    res_header_separator++;

    char * res_header_end = strstr(first_header_row, "\n");
    if (res_header_end == NULL) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    memcpy(res_y_ascii, res_header_separator, (res_header_end - res_header_separator));

    int image_width, image_height;
    if (!sscanf(res_x_ascii, "%i", & image_width)) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (!sscanf(res_y_ascii, "%i", & image_height)) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    char * image_buffer = strstr(res_header_end, "\n");
    if (image_buffer == NULL) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    image_buffer++;

    image_buffer = strstr(image_buffer, "\n");
    if (image_buffer == NULL) {
      std::cerr << "Error parsing input image header!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }
    image_buffer++;

    int bytes_per_row = image_width * (image_bw ? 1 : 3);

    int padded_image_size = bytes_per_row * (image_height + 2);
    char * padded_image_buffer = new(std::nothrow) char[padded_image_size];
    if (padded_image_buffer == NULL) {
      std::cerr << "Unable to allocate memory for image!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    memset(padded_image_buffer, 0, bytes_per_row);
    memcpy(padded_image_buffer + bytes_per_row, image_buffer, bytes_per_row * image_height);
    memset(padded_image_buffer + (bytes_per_row * image_height) + bytes_per_row, 0, bytes_per_row);

    delete[] image_file_buffer;
    image_buffer = padded_image_buffer;

    int rows_per_thread = image_height / NThreads;

    int part_image_size = rows_per_thread * bytes_per_row;

    char * part_image_buffer = image_buffer;

    if (NThreads > 1) {
      for (int i = 1; i < NThreads; i++) {
        MPI_Send((void * ) & image_bw, 1, MPI_BYTE, i, 0, MPI_COMM_WORLD);
        MPI_Send((void * ) & image_width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send((void * ) & rows_per_thread, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        MPI_Send(part_image_buffer, part_image_size + (bytes_per_row * 2), MPI_BYTE, i, 0, MPI_COMM_WORLD);

        part_image_buffer += part_image_size;
      }

    }

    int working_image_size = (image_height * bytes_per_row) - (part_image_buffer - image_buffer);
    int working_rows = working_image_size / bytes_per_row;

    char * output_image_part = new(std::nothrow) char[working_image_size];
    if (output_image_part == NULL) {
      std::cerr << "Unable to allocate memory for working image!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    for (int i = 3; i < argc - 1; i++) {
      apply_filter(argv[i], part_image_buffer + bytes_per_row, output_image_part, image_width, working_rows, image_bw);
      memcpy(part_image_buffer + bytes_per_row, output_image_part, working_image_size);

      if (NThreads > 1) {
        MPI_Send(part_image_buffer + bytes_per_row, bytes_per_row, MPI_BYTE, NThreads - 1, 0, MPI_COMM_WORLD);
        MPI_Recv(part_image_buffer, bytes_per_row, MPI_BYTE, NThreads - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }

    }

    apply_filter(argv[argc - 1], part_image_buffer + bytes_per_row, output_image_part, image_width, working_rows, image_bw);
    memcpy(part_image_buffer + bytes_per_row, output_image_part, working_image_size);

    if (NThreads > 1) {
      for (int i = 1; i < NThreads; i++) {
        MPI_Recv(image_buffer + bytes_per_row + ((i - 1) * rows_per_thread * bytes_per_row), rows_per_thread * bytes_per_row, MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }

    char resolution_ascii[64];
    std::ofstream output_image_file;
    output_image_file.open(argv[2], std::ios::binary);
    if (!output_image_file.is_open()) {
      std::cerr << "Unable to open output file!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (image_bw) {
      output_image_file.write("P5\n", 3);
    } else {
      output_image_file.write("P6\n", 3);
    }

    sprintf(resolution_ascii, "%d %d\n", image_width, image_height);

    output_image_file.write(resolution_ascii, strlen(resolution_ascii));
    output_image_file.write("255\n", 4);

    output_image_file.write(image_buffer + bytes_per_row, image_height * bytes_per_row);
    output_image_file.close();
  } else {
    bool image_bw;
    int image_width, recv_image_height;

    MPI_Recv((void * ) & image_bw, 1, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv((void * ) & image_width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv((void * ) & recv_image_height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    int bytes_per_row = image_width * (image_bw ? 1 : 3);

    int working_image_size = (recv_image_height + 2) * bytes_per_row;
    char * input_image_part = new(std::nothrow) char[working_image_size * 2];
    if (input_image_part == NULL) {
      std::cerr << "Unable to allocate memory for working image part!" << std::endl;
      MPI_Abort(MPI_COMM_WORLD, -1);
    }

    char * output_image_part = input_image_part + working_image_size;

    MPI_Recv(input_image_part, working_image_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (int i = 3; i < argc - 1; i++) {
      apply_filter(argv[i], input_image_part + bytes_per_row, output_image_part, image_width, recv_image_height, image_bw);
      memcpy(input_image_part + bytes_per_row, output_image_part, working_image_size - 2 * bytes_per_row);

      MPI_Recv(input_image_part + working_image_size - bytes_per_row, bytes_per_row, MPI_BYTE, (thread_id + 1) % NThreads, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(input_image_part + working_image_size - 2 * bytes_per_row, bytes_per_row, MPI_BYTE, (thread_id + 1) % NThreads, 0, MPI_COMM_WORLD);

      if (thread_id != 1) {
        MPI_Send(input_image_part + bytes_per_row, bytes_per_row, MPI_BYTE, thread_id - 1, 0, MPI_COMM_WORLD);
        MPI_Recv((void * ) input_image_part, bytes_per_row, MPI_BYTE, thread_id - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }

    }

    apply_filter(argv[argc - 1], input_image_part + bytes_per_row, output_image_part, image_width, recv_image_height, image_bw);
    memcpy(input_image_part + bytes_per_row, output_image_part, working_image_size - 2 * bytes_per_row);

    MPI_Send(input_image_part + bytes_per_row, working_image_size - 2 * bytes_per_row, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}