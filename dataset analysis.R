#After having started the analysis with the SourceMe.R file
library(data.table)
##example("data.table")
data <- summary()
data<-data.table(data$agents)
data

#VISUALIZING THE AVERAGE FITNESS OF POP
plot(data[,`pop fitness`], type = "l")
#considering only the first "n" genrations
fit <- data[,`pop fitness`][c(1:1000)]
plot(fit, type = "l")


#VISUALIZING THE NUMBER OF REPRODUCTIVE CLADES PER GEN
plot(data[,`repro clades`], type = "l")
#considering only the first "n" genrations
clad <- data[,`repro clades`][c(1:1000)]
plot(clad, type = "l")


#VISUALIZING THE NUMBER OF REPRODUCTIVE INDIVIDUALS PER GEN
plot(data[,`repro ind`], type = "l")
#considering only the first "n" genrations
repro_ind <- data[,`repro ind`][c(1:1000)]
plot(repro_ind, type = "l")

#§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
#Here we want to visualize the number of agents which have the same fitness in each generation

gen_list <- list()    #creating the empty list

#Let's consider only the first 120 generations, every 5 generations 
for (i in seq(1,120,5)){
  temp_table <- generation(i)
  temp_table <- data.table(temp_table$agents$fit)
  temp_table[,gen:=rep(i,nrow(temp_table))]
  gen_list[[as.character(i)]] <- temp_table
}

#We have to load the package Tidyverse because I'm still not able to do the same with data.table
library(tidyverse)
for (i in seq_along(gen_list)){
  gen_list[[i]] <- as_tibble(gen_list[[i]])
}

#useless stuff
# a <- gen_list[[1]]
# ggplot(a, aes(V1))+
#   geom_histogram(binwidth = 1)
# b <- gen_list[[2]]
# ggplot(b, aes(V1))+
#   geom_histogram(binwidth = 1)

gen_list_cool <- bind_rows(gen_list)
gen_list_cool

#plotting the histogram of the number of agents with the same fitness
#divided (wrapped) per generation
ggplot(gen_list_cool,aes(V1))+
  geom_histogram(binwidth = 1)+
  facet_wrap(vars(gen))

#§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
#The objective here is to make a clustering of all the different ANN that exist in each generation

#once again we create a list for storing the ANN:
gen_list_ann <- list()
for (i in seq(1,120,5)){
  temp_table_ann <- generation(i)
  temp_table_ann <- data.table(temp_table_ann$agents$ann)
  gen_list_ann[[as.character(i)]] <- temp_table_ann
}

#Here we decided to just look how this was possible for one particular generation (10)
x <- as_tibble(gen_list_ann[[10]])
y <- gen_list[[10]]

w <- x %>% 
  group_by(V1,V2,V3,V4,V5,V6,V7,V8) %>% 
  summarise(n_ann=n())

ww <- w %>% 
  select(-n_ann)

my_ann_data <- as.matrix(ww)

#Loading the package(stats) for the clustering analysis
#library(stats)
d <- dist(my_ann_data, method = "euclidean")    #matrix of distances
diff_ann <- hclust(d, method="ward.D")          #clustering (hclust objects)
plot(diff_ann) # display dendogram
groups_ann <- cutree(diff_ann, k=5) # cut tree into 5 clusters
# draw dendogram with red borders around the 5 clusters 
rect.hclust(diff_ann, k=5, border="red")


#§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

parents <- list()
for (i in 0:dim(data)[1]){
  a <- generation(i)
  b <- a$agents$anc
  parents[[as.character(i)]] <- length(unique(b))
}
parents

ar <- vector()
for (i in seq_along(parents)){
  ar[i] <- parents[[i]][1]
}

plot(ar, type = "l")
