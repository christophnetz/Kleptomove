## Script N5 after sourcing the SourceMe file
#This script is for creating some MORE COMPLEX plots, starting from a more extended dataset.
#In this script we analyze the ANN weights and we visualize the changes during the simulation time

#creating an empty list for containing the ANNs of each generation considered
ann_list <- list()

#filling the list
for (i in 1:length(list_gen)){
  ann_list[[i]] <- tibble(
    gen = c(rep(g[i],length(list_gen[[i]]$fit))),
    id = c(1:length(list_gen[[i]]$fit)),
    w1 = list_gen[[i]]$ann[,1],
    w2 = list_gen[[i]]$ann[,2],
    w3 = list_gen[[i]]$ann[,3],
    w4 = list_gen[[i]]$ann[,4],
    w5 = list_gen[[i]]$ann[,5],
    w6 = list_gen[[i]]$ann[,6],
    w7 = list_gen[[i]]$ann[,7],
    w8 = list_gen[[i]]$ann[,8])}

#reshaping the list for creating a single dataframe
ann_df <- dplyr::bind_rows(ann_list)

#gathering all the weights
ann0 <- ann_df %>% 
  gather(w1,w2,w3,w4,w5,w6,w7,w8, key="node", value="weight")

#manipulating the dataframe
ann1 <- ann0 %>% 
  mutate(bin=plyr::round_any(weight,0.01)) %>% 
  group_by(gen,node,bin) %>% 
  summarize(count=n())

#PLOT
#This plot show how the weights of all the artificial neural netwoks change (or shift)
#towards some value during the simulation (thorugh generations).
ggplot(ann1,aes(x=gen, y=bin, fill=count))+
  geom_tile()+
  theme_bw()+
  facet_wrap(~node, scale="free")+
  #ylim(-1.5,1.5)+
  ylim(-1.5,1.5)+
  scale_fill_gradient2(low="white",high = "blue",mid = "red", midpoint = 2500, limit=c(0,10000))
  #scale_fill_viridis_c(direction = -1)
  #scale_fill_distiller(palette="Blues", direction = 1)