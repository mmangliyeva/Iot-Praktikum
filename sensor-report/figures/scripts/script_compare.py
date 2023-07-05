import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
mapFiles = {"group1":{"count":"value.count"},
            "group2":{"count":"value.count"},
            "group4":{"count":"value.count"},
            "group8":{"count":"value.countPeople"},
            "group9":{"count":"value.peopleCount"},
            "group10":{"count":"value.count"},
            "group11":{"count":"value.count"},
            "group13":{"count":"value.count"},
            "group15":{"count":"value.count"},
            }
def diff_to_ref(df, name):
    df = df.fillna(0)
    df = df.filter(["reference", name])
    df['diff'] = (df['reference'] - df[name]).abs()
    absolute_diff = df["diff"].sum()
    df = df.drop(columns='diff')
    return absolute_diff

df = pd.read_csv("groupComparison_max.csv")
df["Datetime"] = pd.to_datetime(df["Datetime"])
df.set_index("Datetime", inplace=True)

for name in mapFiles:
    # print(name, diff_to_ref(df,name))
    print_df = df.filter(["reference", name])
    ax = print_df.plot( xlabel="time", ylabel="count",title=name+"\nAbsolute difference: "+("%.2f" % diff_to_ref(df,name)))
    ax.figure.savefig('ref-'+name+'.jpeg')

# print(df)
# ticklabels = df.index.strftime('%d')

# ax = df.plot(subplots=True, layout=(2,5), xlabel="time")
# plt.tight_layout()
# plt.show()



