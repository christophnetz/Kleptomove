## Script N2 after sourcing the SourceMe file
#This script is for creating some BASIC plots:

#Plot different actions (proportions per generations)
act <- ggplot(data)+
	geom_line(aes( y= foraging/1000000, x= 1:dim(data)[1] ), col="blue")+
	geom_line(aes( y= handling/1000000, x= 1:dim(data)[1] ), col="green")+
	geom_line(aes( y= stealing/1000000, x= 1:dim(data)[1] ), col="red")

act

act_smooth <- ggplot(data)+
	geom_smooth(aes( y= foraging/1000000, x= 1:dim(data)[1] ), col="blue")+
	geom_smooth(aes( y= handling/1000000, x= 1:dim(data)[1] ), col="green")+
	geom_smooth(aes( y= stealing/1000000, x= 1:dim(data)[1] ), col="red")

act_smooth	

#---------------------------------------------------------------
#Plot average fitness
avg_fit <- ggplot(data)+
	geom_line(aes( y= `pop fitness`, x= 1:dim(data)[1] ))

avg_fit

#----------------------------------------------------------------
#Stealing efficiency trough time (generations)
stl_eff <- ggplot(data)+
  geom_point(aes(y=conflicts/stealing,x=1:dim(data)[1]), na.rm=TRUE)

stl_eff

#------------------------------------------------------------------
