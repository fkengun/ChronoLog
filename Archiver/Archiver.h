class archiver {
  public:
    // Constructor
    archiver(std::string a_name, std::vector<std::string> b_names, uint64_t start_t, uint64_t end_t)
      : a_name(a_name),
      b_names(b_names),
      start_t(start_t),
      end_t(end_t) {}

    // Write function
    void write(std::vector<struct E> E_chunks) {
      for (auto b_name : b_names) {
        // Create the HDF5 file if it does not exist
        hid_t file_id = H5Fcreate(
            a_name + "_" + b_name + "." + std::to_string(start_t) + "." + std::to_string(end_t) + ".h5",
            H5F_ACC_TRUNC, H5F_DEFAULT, H5P_DEFAULT);

        // Create the dataset
        hid_t dataset_id = H5Dcreate(
            file_id, b_name.c_str(), H5T_STD_U64LE, H5S_UNLIMITED, H5P_DEFAULT, H5P_DEFAULT);

        // Write the chunks to the dataset
        for (auto chunk : E_chunks) {
          // Get the char* from the struct
          char* data = chunk.data;

          // Write the char* to the dataset
          H5Dwrite(dataset_id, H5T_STD_U64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
        }

        // Close the dataset
        H5Dclose(dataset_id);

        // Close the file
        H5Fclose(file_id);
      }
    }

  private:
    // The name of the archive
    std::string a_name;

    // The names of the datasets
    std::vector<std::string> b_names;

    // The start time of the archive
    uint64_t start_t;

    // The end time of the archive
    uint64_t end_t;
};
