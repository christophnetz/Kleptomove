## Script N4 after sourcing the SourceMe file
#This script is for creating some MORE COMPLEX plots, starting from a more extended dataset.

#we start from the "w" element created in the previous script

#defining funciton to summarize the mean of time spent doing the 3 activity
b_fun <- function(x) {summarise(x, f=mean(p_ft),h=mean(p_ht), s=mean(p_st), tot=f+h+s)}

#mapping w with the previous function
b0 <- map(w,b_fun)	#takes a list ("w"), create a list ("b0")

#creating a single dataframe out of the list "b0"
b1 <- map2_df(b0, g, function(i,j){mutate(i,gen=j)})

#gathering activities
b2 <- b1 %>% 
  gather(f,h,s, key = "Activity", value="prop")

#PLOT
#plotting on the x-axis the generations in consideration (in the vector "g")
#while on the y axis we have in different colors the proportion that each activity has taken in each generation
ggplot(b2)+
  geom_bar(aes(x=gen, y=prop, fill=Activity), stat="identity")+
  #scale_fill_manual(values = c("deepskyblue4", "chartreuse4", "firebrick3"), labels = c("foragers", "handlers", "stealers"))+
  scale_fill_brewer(labels = c("foragers", "handlers", "stealers"))+
  labs(x="Generations", y="Proportion of Population", title = "Proportion of Agents per activity")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(-10,510))+
  scale_y_continuous(labels = scales::percent)+
  my_theme
