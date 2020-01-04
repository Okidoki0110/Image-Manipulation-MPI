#ifndef _FILTER_S_H
#define _FILTER_S_H

#include <cstdint>


double smooth_matrix[3][3] = {
  {
    0.11111111111,
    0.11111111111,
    0.11111111111
  },
  {
    0.11111111111,
    0.11111111111,
    0.11111111111
  },
  {
    0.11111111111,
    0.11111111111,
    0.11111111111
  }
};

double blur_matrix[3][3] = {
  {
    0.0625,
    0.125,
    0.0625
  },
  {
    0.125,
    0.25,
    0.125
  },
  {
    0.0625,
    0.125,
    0.0625
  }
};

double sharpen_matrix[3][3] = {
  {
    0,
    -0.66666666666,
    0
  },
  {
    -0.66666666666,
    3.66666666666,
    -0.66666666666
  },
  {
    0,
    -0.66666666666,
    0
  }
};

double mean_matrix[3][3] = {
  {
    -1, -1, -1
  },
  {
    -1,
    9,
    -1
  },
  {
    -1,
    -1,
    -1
  }
};

double emboss_matrix[3][3] = {
  {
    0,
    1,
    0
  },
  {
    0,
    0,
    0
  },
  {
    0,
    -1,
    0
  }
};

double identity_matrix[3][3] = {
  {
    0,
    0,
    0
  },
  {
    0,
    1,
    0
  },
  {
    0,
    0,
    0
  }
};

inline uint8_t compute_convolute_filter(double convolute_matrix[3][3], uint8_t I11, uint8_t I12, uint8_t I13, uint8_t I21, uint8_t I22, uint8_t I23, uint8_t I31, uint8_t I32, uint8_t I33) {
  double r = (convolute_matrix[2][2] * I11) + (convolute_matrix[2][1] * I12) + (convolute_matrix[2][0] * I13) + (convolute_matrix[1][2] * I21) + (convolute_matrix[1][1] * I22) +
    (convolute_matrix[1][0] * I23) + (convolute_matrix[0][2] * I31) + (convolute_matrix[0][1] * I32) + (convolute_matrix[0][0] * I33);

  if (r < 0) {
    return 0;
  } else if (r > 255) {
    return 255;
  }
  return r;
}

void copy_matrix(double dest_matrix[3][3], double source_matrix[3][3]) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      dest_matrix[i][j] = source_matrix[i][j];
    }
  }
}

inline uint8_t get_pixel_bw(int x, int y, int width, char * image_buffer) {
  if (x < 0 or x >= width) {
    return 0;
  }
  return image_buffer[(y * width) + x];
}

inline uint8_t get_pixel_r(int x, int y, int width, char * image_buffer) {
  if (x < 0 or x >= width) {
    return 0;
  }
  return image_buffer[((y * width) + x) * 3];
}

inline uint8_t get_pixel_g(int x, int y, int width, char * image_buffer) {
  if (x < 0 or x >= width) {
    return 0;
  }
  return image_buffer[(((y * width) + x) * 3) + 1];
}

inline uint8_t get_pixel_b(int x, int y, int width, char * image_buffer) {
  if (x < 0 or x >= width) {
    return 0;
  }
  return image_buffer[(((y * width) + x) * 3) + 2];
}

inline void apply_convolute_matrix(double convolute_matrix[3][3], char * input, char * output, int x, int y, int width, bool bw) {
  uint8_t I11, I12, I13, I21, I22, I23, I31, I32, I33;

  if (bw) {
    I11 = get_pixel_bw(x - 1, y - 1, width, input);
    I12 = get_pixel_bw(x, y - 1, width, input);
    I13 = get_pixel_bw(x + 1, y - 1, width, input);

    I21 = get_pixel_bw(x - 1, y, width, input);
    I22 = get_pixel_bw(x, y, width, input);
    I23 = get_pixel_bw(x + 1, y, width, input);

    I31 = get_pixel_bw(x - 1, y + 1, width, input);
    I32 = get_pixel_bw(x, y + 1, width, input);
    I33 = get_pixel_bw(x + 1, y + 1, width, input);

    output[y * width + x] = compute_convolute_filter(convolute_matrix, I11, I12, I13, I21, I22, I23, I31, I32, I33);
  } else {
    I11 = get_pixel_r(x - 1, y - 1, width, input);
    I12 = get_pixel_r(x, y - 1, width, input);
    I13 = get_pixel_r(x + 1, y - 1, width, input);

    I21 = get_pixel_r(x - 1, y, width, input);
    I22 = get_pixel_r(x, y, width, input);
    I23 = get_pixel_r(x + 1, y, width, input);

    I31 = get_pixel_r(x - 1, y + 1, width, input);
    I32 = get_pixel_r(x, y + 1, width, input);
    I33 = get_pixel_r(x + 1, y + 1, width, input);

    output[(y * width + x) * 3] = compute_convolute_filter(convolute_matrix, I11, I12, I13, I21, I22, I23, I31, I32, I33);

    I11 = get_pixel_g(x - 1, y - 1, width, input);
    I12 = get_pixel_g(x, y - 1, width, input);
    I13 = get_pixel_g(x + 1, y - 1, width, input);

    I21 = get_pixel_g(x - 1, y, width, input);
    I22 = get_pixel_g(x, y, width, input);
    I23 = get_pixel_g(x + 1, y, width, input);

    I31 = get_pixel_g(x - 1, y + 1, width, input);
    I32 = get_pixel_g(x, y + 1, width, input);
    I33 = get_pixel_g(x + 1, y + 1, width, input);

    output[((y * width + x) * 3) + 1] = compute_convolute_filter(convolute_matrix, I11, I12, I13, I21, I22, I23, I31, I32, I33);

    I11 = get_pixel_b(x - 1, y - 1, width, input);
    I12 = get_pixel_b(x, y - 1, width, input);
    I13 = get_pixel_b(x + 1, y - 1, width, input);

    I21 = get_pixel_b(x - 1, y, width, input);
    I22 = get_pixel_b(x, y, width, input);
    I23 = get_pixel_b(x + 1, y, width, input);

    I31 = get_pixel_b(x - 1, y + 1, width, input);
    I32 = get_pixel_b(x, y + 1, width, input);
    I33 = get_pixel_b(x + 1, y + 1, width, input);

    output[((y * width + x) * 3) + 2] = compute_convolute_filter(convolute_matrix, I11, I12, I13, I21, I22, I23, I31, I32, I33);
  }

}

void apply_filter(char * filter_name, char * input_image, char * output_image, int image_width, int image_height, bool image_bw) {
  double filter_matrix[3][3];
  copy_matrix(filter_matrix, identity_matrix);

  if (strcmp("smooth", filter_name) == 0) {
    copy_matrix(filter_matrix, smooth_matrix);
  } else if (strcmp("blur", filter_name) == 0) {
    copy_matrix(filter_matrix, blur_matrix);
  } else if (strcmp("sharpen", filter_name) == 0) {
    copy_matrix(filter_matrix, sharpen_matrix);
  } else if (strcmp("mean", filter_name) == 0) {
    copy_matrix(filter_matrix, mean_matrix);
  } else if (strcmp("emboss", filter_name) == 0) {
    copy_matrix(filter_matrix, emboss_matrix);
  }

  for (int i = 0; i < image_height; i++) {

    for (int j = 0; j < image_width; j++) {
      apply_convolute_matrix(filter_matrix, input_image, output_image, j, i, image_width, image_bw);
    }

  }

}

#endif