import pandas as pd

files = ["Blatt 1-1) Booting", "Blatt 2-2) Wifi", "Blatt 3-3) Calculation",
         "Blatt 4-4) Parallization","Blatt 5-5) Light sleep & 6) Deep sleep"]

for filename in files:
    csv_table = pd.read_csv(filename+".csv",sep=';')
    
    # print(csv_table.iloc[-1:].head())
    lastRow = csv_table.iloc[-1:]
    
    with open(filename+"_average.tex", 'w') as f:
        f.write(lastRow.to_latex(index=False))