## Script N2 after sourcing the SourceMe file
#This script is for creating some BASIC plots:

#Plot different actions (proportions per generations)
act <- ggplot(data)+
	geom_line(aes( y= foraging/1000000, x= 1:dim(data)[1] ), col="blue")+
	geom_line(aes( y= handling/1000000, x= 1:dim(data)[1] ), col="green")+
	geom_line(aes( y= stealing/1000000, x= 1:dim(data)[1] ), col="red")+
  labs(x="Generations", y="prop activity", title = "Proportion of Individuals grouped for activity")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
  scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = waiver())+
  my_theme
  

#act

act_smooth <- ggplot(data)+
	geom_smooth(aes( y= foraging/1000000, x= 1:dim(data)[1] ), col="blue")+
	geom_smooth(aes( y= handling/1000000, x= 1:dim(data)[1] ), col="green")+
	geom_smooth(aes( y= stealing/1000000, x= 1:dim(data)[1] ), col="red")+
  labs(x="Generations", y="prop activity", title = "Proportion of Individuals grouped for activity")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
  scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = waiver())+
  my_theme

#act_smooth	

#---------------------------------------------------------------
#Plot average fitness
avg_fit <- ggplot(data)+
	geom_line(aes( y= `pop fitness`, x= 1:dim(data)[1] ))+
  labs(x="Generations", y="average fitness", title = "Population fitness", subtitle = "visualization of the average value over time")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
  scale_y_continuous(breaks = seq(0,20,1), minor_breaks = NULL, limits = c(0, NA))+
  my_theme

avg_fit

#----------------------------------------------------------------
#Stealing efficiency trough time (generations)
stl_eff <- ggplot(data)+
  geom_line(aes(y=conflicts/stealing,x=1:dim(data)[1]), na.rm=TRUE)+
  labs(x="Generations", y="stealing success/stealing attempt", title = "Effectiveness of stealing")+
  scale_x_continuous(breaks = seq(0,1000,100), minor_breaks = NULL, limits = c(0,1000))+
  scale_y_continuous(breaks = seq(0,1,0.1), minor_breaks = waiver())+
  my_theme

#stl_eff

#------------------------------------------------------------------
