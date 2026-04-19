// Design for optimizing on-board storage of environmental data logger data
// on an 8kB flash memory

#include "storage_techniques.cc"

float rand_1_decimal(int upper) {
    return (float)(rand() % (10*upper)) / 10;
}

// Choose which write scheme to use when writing this block of data
void write_data(string fname, vector<DataPoint> data, 
    std::function<void(string, u_int32_t, float, float)> write_function) {
    
    // Empty file before write
    ofstream fout(fname);
    fout.close();
    
    // Do this one at a time to simulate the data logger logging a new point every 5 minutes
    for (const DataPoint& point : data)
        write_function(fname, point.timestamp, point.temperature, point.humidity);
}

// Choose which scheme to use when reading this block of data
vector<DataPoint> read_data(string fname, int num_datapoints,
    std::function<vector<DataPoint>(string, int)> read_function) {

    return read_function(fname, num_datapoints);
}

// Simulate real data generation
vector<DataPoint> generate_data(int num_datapoints) {
    vector<DataPoint> data(num_datapoints);
    
    u_int32_t unix_timestamp = 1775410200; // Timestamp for 2026-04-05 17:30:00
    for (int i=0; i<num_datapoints; ++i) {
        data[i].timestamp   = unix_timestamp;
        data[i].temperature = rand_1_decimal(100); // Random temperature value
        data[i].humidity    = rand_1_decimal(100); // Random humidity value
        unix_timestamp += 300; // Increment by 5 minutes
    }
    return data;
}

bool verify(string fname, vector<DataPoint> v1, vector<DataPoint> v2) {
    if (v1 == v2) {
        cout << fname << " is functionally correct with size: ";
        cout << std::filesystem::file_size(fname) << " bytes" << endl;
        return true;
    }
    cerr << fname << " is INCORRECT with size: ";
    cout << std::filesystem::file_size(fname) << " bytes" << endl;
    return false;
}

int main() {

    int num_datapoints = 3000;
    
    vector<DataPoint> data_to_write = generate_data(num_datapoints);
    vector<DataPoint> data_read_from_file;

    write_data("file1.dat", data_to_write, write_naive);
    data_read_from_file = read_data("file1.dat", num_datapoints, read_naive);
    verify("file1.dat", data_to_write, data_read_from_file);
    
    write_data("file2.dat", data_to_write, write_struct);
    data_read_from_file = read_data("file2.dat", num_datapoints, read_struct);
    verify("file2.dat", data_to_write, data_read_from_file);
    
    write_data("file3.dat", data_to_write, write_compressed);
    data_read_from_file = read_data("file3.dat", num_datapoints, read_compressed);
    verify("file3.dat", data_to_write, data_read_from_file);

    return 0;
}
