#include <cstdlib>
#include <cstring>
#include "hdf5.h"

int main() {
  // Open the HDF5 file
  hid_t file_id = H5Fcreate("myfile.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  // Create a dataset in the file
  hid_t dataset_id = H5Dcreate(
      file_id, "mydataset", H5T_COMPOUND, H5S_UNLIMITED, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  // Create a for loop to write to the dataset
  for (int i = 1; i < 2; i++) {
    // Create a new compound datatype
    hid_t compound_datatype_id = H5Tcreate(H5T_COMPOUND, i * 10);

    // Add a field to the compound datatype
    hsize_t field_size = i * 10;
    char* field_name = "field";
    H5Tinsert(compound_datatype_id, field_name, 0, field_size);

    // Write the element to the dataset
    void* data = malloc(field_size);
    memset(data, i, field_size);
    H5Dwrite(dataset_id, compound_datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    // Free the data
    free(data);

    // Close the datatype
    H5Tclose(compound_datatype_id);
  }

  // Close the dataset
  H5Dclose(dataset_id);

  // Close the file
  H5Fclose(file_id);

  return 0;
}
