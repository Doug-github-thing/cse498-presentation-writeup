with open("input_file.txt", 'w') as file:
  for i in range(100000):
    file.write(f"line {i:5}\n")
