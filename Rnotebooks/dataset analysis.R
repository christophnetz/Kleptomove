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

#Looking at foraging and handling events
hand <- data[,`handling`][c(1:120)]
forag <- data[,`foraging`][c(1:120)]
con <- data[,`conflicts`][c(1:120)]

plot(hand, type = "l", col="red", ylim = c(0,680000))
lines(forag,type = "l", col="green")

par(new=TRUE)
plot(con, type = "l", axes = FALSE)
axis(side = 4)

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
data[,stealing:=1000000-(foraging+handling)]
data
kk <- data[, c("handling", "foraging", "stealing", "conflicts")]
kk[, rapporto:=stealing/conflicts]
rap <- kk[,`rapporto`]
plot(rap)

#here we visualize something weird:
#the plot shows on the x from 0 to 1000 the descrease in the ratio of stealing individuals and conflicts
#while on the y we have the generation that has that particular ratio.
desc_rap <- kk[,generation:=c(1:1000)]
desc_rap <- desc_rap[order(-rapporto)]
desc_rap <- desc_rap[,generation]
plot(desc_rap, type="l")

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
ar
plot(ar, type = "l")

## I think this version is better (if it works)
# parents <- vector()
# for (i in 0:dim(data)[1]){
#   a <- generation(i)
#   b <- a$agents$anc
#   parents[i] <- length(unique(b))
# }
# parents

#Checking out in a specific generation some stuff
p <- generation(23)
pfoa <- p$agents$foa
phan <- p$agents$han
pstl <- 100-(pfoa+phan)

pstl
plot(pstl,type = "h")#visualize number of time individuals tried to steal

library(tidyverse)
pstl_tidy <- as_tibble(pstl)
zum <- pstl_tidy %>% 
  mutate(id=c(1:nrow(pstl_tidy))) %>% 
  group_by(value) %>% 
  summarize(num=n_distinct(id))

arrange(zum, desc(value))

b <- pstl_tidy %>% 
  mutate(id=c(1:nrow(pstl_tidy)))

