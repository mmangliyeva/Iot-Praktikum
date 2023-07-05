import pandas as pd

timestampName = "Datetime"
mapFiles = {"reference":{"count":"value.count"},
            "group1":{"count":"value.count"},
            "group2":{"count":"value.count"},
            "group4":{"count":"value.count"},
            "group8":{"count":"value.countPeople"},
            "group9":{"count":"value.peopleCount"},
            "group10":{"count":"value.count"},
            "group11":{"count":"value.count"},
            "group13":{"count":"value.count"},
            "group15":{"count":"value.count"},
            }

group_df = {}

for fileName in mapFiles:
    curr_df = pd.read_csv(fileName+".csv")
    curr_df = curr_df.filter(['timestamp', mapFiles[fileName]["count"]])
    curr_df[timestampName] = pd.to_datetime(curr_df.timestamp, format="%b %d, %Y @ %H:%M:%S.%f")
    curr_df = curr_df.drop(['timestamp'], axis=1)
    curr_df = curr_df.sort_values(by=timestampName,ascending=True)
    curr_df.set_index([timestampName], inplace=True)
    # curr_df = curr_df.reset_index(drop=True)

    group_df[fileName] = curr_df


comparision_df = pd.DataFrame()
comparision_df["reference"] = group_df["reference"]


for i in range(0,len(group_df["reference"])-1):
    #iterate over all referecnes times
    start_date = comparision_df.index[i]
    end_date = comparision_df.index[i+1]
    for df in group_df:
        if df != "reference":
            #iterate over all to compared dfs
            df_in_timeRange = group_df[df][(group_df[df].index >= start_date) & (group_df[df].index <=end_date)]
            # print("selected timertange:", df_in_timeRange.head())
            # comparision_df[df][i] = 
            mean = df_in_timeRange[mapFiles[df]["count"]].max()
            # print('mean', mean)
            comparision_df.loc[comparision_df.index[i], df] = mean

# print(comparision_df.head())
comparision_df.to_csv("groupComparison_max.csv")




