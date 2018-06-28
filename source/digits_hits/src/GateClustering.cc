

#include "GateClustering.hh"

#include "G4UnitsTable.hh"
#include "GateVolumeID.hh"
#include "GateClusteringMessenger.hh"
#include "G4Electron.hh"
#include "GateConfiguration.h"
#include "GateObjectStore.hh"
#include "GateConstants.hh"
#include "GateTools.hh"

#include "GateVVolume.hh"
#include "G4VoxelLimits.hh"




GateClustering::GateClustering(GatePulseProcessorChain* itsChain,
                                                               const G4String& itsName)
    : GateVPulseProcessor(itsChain,itsName)
{

 m_flgMRejection=0;
 m_messenger = new GateClusteringMessenger(this);




}

GateClustering::~GateClustering()
{
	delete m_messenger;
}





void GateClustering::ProcessOnePulse(const GatePulse* inputPulse,GatePulseList& outputPulseList)
{


    GatePulse* outputPulse = new GatePulse(*inputPulse);
    std::vector<double> dist;
    std::vector<int> index4ClustSameVol;

                 if(outputPulseList.empty()){
                     outputPulseList.push_back(outputPulse);

                 }
                 else{

                     for(unsigned int i=0; i<outputPulseList.size();i++){
                         if(outputPulse->GetVolumeID() == (outputPulseList.at(i))->GetVolumeID() && outputPulse->GetEventID()==(outputPulseList.at(i))->GetEventID() ){

                           dist.push_back(getDistance(outputPulse->GetGlobalPos(),outputPulseList.at(i)->GetGlobalPos()));
                           index4ClustSameVol.push_back(i);
                         }
                     }

                     if(dist.size()>0){
                         std::vector<double>::iterator itmin = std::min_element(std::begin(dist), std::end(dist));

                         unsigned int posMin=std::distance(std::begin(dist), itmin);
                         if(dist.at(posMin)< m_acceptedDistance){
                             //Sum the hit to the cluster. sum of energies, position (global and local) weighted in energies, min time
                             outputPulseList.at( index4ClustSameVol.at(posMin))->CentroidMerge(outputPulse);

                         }
                         else{
                             outputPulseList.push_back(outputPulse);

                         }
                     }
                     else{
                         outputPulseList.push_back(outputPulse);
                     }


                 }


}


GatePulseList* GateClustering::ProcessPulseList(const GatePulseList* inputPulseList)
{
  if (!inputPulseList)
    return 0;

  size_t n_pulses = inputPulseList->size();
  if (nVerboseLevel==1)
        G4cout << "[" << GetObjectName() << "::ProcessPulseList]: processing input list with " << n_pulses << " entries\n";
  if (!n_pulses)
    return 0;

  GatePulseList* outputPulseList = new GatePulseList(GetObjectName());

 //G4cout<<" ini event"<<G4endl;

  GatePulseConstIterator iter;
  for (iter = inputPulseList->begin() ; iter != inputPulseList->end() ; ++iter)
  //{
      ProcessOnePulse( *iter, *outputPulseList);
      //G4cout<<" pulses"<<G4endl;
  //}
 // G4cout<<" end pulses"<<G4endl;

// if(outputPulseList){
//  G4cout<<"cluster number="<<outputPulseList->size()<<G4endl;
//  if(outputPulseList->size()>0)
//  G4cout<<"eventID"<<outputPulseList->at(0)->GetEventID()<<G4endl;
// }

checkClusterCentersDistance(*outputPulseList);
if(m_flgMRejection==1){
    //G4cout<<"rejection activated"<<G4endl;
    //Chequear si hay multiples
 if(outputPulseList->size()>1){
     bool flagM=0;

     for(unsigned int i=0; i<outputPulseList->size()-1; i++){
              for(unsigned int k=i+1;k<outputPulseList->size(); k++){
                  if(outputPulseList->at(i)->GetVolumeID() == outputPulseList->at(k)->GetVolumeID()){
                      flagM=1;
                      break;

                  }
              }
              if(flagM==1)break;
     }
     if(flagM==1){
            while (outputPulseList->size())
           {
               delete outputPulseList->back();
               outputPulseList->erase(outputPulseList->end()-1);
             }
     }

      //Queria hacerlo asi pero tengo un break ahi dentro tras 30000 eventos que no entiendo
    /*sort( outputPulseList->begin( ), outputPulseList->end( ), [ ]( const GatePulse& pulse1, const GatePulse& pulse2 )
    {
        if(pulse1.GetGlobalPos().getZ() == pulse1.GetGlobalPos().getZ())
                        return fabs(pulse1.GetGlobalPos().getX() )<fabs(pulse2.GetGlobalPos().getX());
        return pulse1.GetGlobalPos().getZ() <pulse2.GetGlobalPos().getZ();

    });

    G4cout<<"sort"<<G4endl;
    auto newEnd = std::unique(outputPulseList->begin(), outputPulseList->end(), [](const GatePulse & first, const GatePulse & sec) {
        if (first.GetVolumeID() == sec.GetVolumeID())
            return true;
        else
            return false;
    });
     G4cout<<"tengo el iterotr, dist "<<std::distance(outputPulseList->begin(),newEnd)<< G4endl;
 G4cout<<"outputsize  fin"<<outputPulseList->size()<<G4endl;
   //if(newEnd!=outputPulseList->end()){
       G4cout<<"Ha encontrado algo repetido"<<G4endl;
//    //asumo volumenes a lo largo de z o funciona igual sin sort ()?
//       //reject the event
//        while (outputPulseList->size())
//          {
//            delete outputPulseList->back();
//            outputPulseList->erase(outputPulseList->end()-1);
//          }
  }*/
 }

}
  if (nVerboseLevel==1) {
      G4cout << "[" << GetObjectName() << "::ProcessPulseList]: returning output pulse-list with " << outputPulseList->size() << " entries\n";
      for (iter = outputPulseList->begin() ; iter != outputPulseList->end() ; ++iter)
        G4cout << **iter << Gateendl;
      G4cout << Gateendl;
  }

  return outputPulseList;
}






void  GateClustering::checkClusterCentersDistance(GatePulseList& outputPulseList){
    //UN POCO CHAPUZA,creo que casos despreciable

    if(!outputPulseList.size()>1){
        std::vector<unsigned int> pos2Delete;
         std::vector<unsigned int>::iterator it;
        //1) Mirar si hay clusters con igual valor de getVolumeID y entonces caluclar las distnacias.
        for(unsigned int i=0; i<outputPulseList.size()-1; i++){
             if(pos2Delete.size()>0){
                 it = find (pos2Delete.begin(), pos2Delete.end(),i);
                 if(it!=pos2Delete.end()){
                     for(unsigned int k=i+1;k<outputPulseList.size(); k++){
                         if(getDistance(outputPulseList.at(i)->GetGlobalPos(),  outputPulseList.at(k)->GetGlobalPos())<m_acceptedDistance){
                             outputPulseList.at(i)->CentroidMerge(outputPulseList.at(k));
                             pos2Delete.push_back(k);
                             G4cout<<" Adding CLUSTERs  since they are CLOSER THAN ACCEPTED DISTANCE  "<<G4endl;
                         }
                     }
                 }
             }
             else{

                 for(unsigned int k=i+1;k<outputPulseList.size(); k++){
                     if(getDistance(outputPulseList.at(i)->GetGlobalPos(),  outputPulseList.at(k)->GetGlobalPos())<m_acceptedDistance){
                         outputPulseList.at(i)->CentroidMerge(outputPulseList.at(k));
                         pos2Delete.push_back(k);
                             G4cout<<" Adding CLUSTERs  since they are CLOSER THAN ACCEPTED DISTANCE  "<<G4endl;
                     }
                 }
             }

        }

        for(unsigned int i=0; i<pos2Delete.size(); i++){

            outputPulseList.erase(outputPulseList.begin()+pos2Delete.at(i)-i);

        }
    }
        // en eso casos calcular


}

void GateClustering::DescribeMyself(size_t  indent)
{

        G4cout << GateTools::Indent(indent) << "Distance of  accepted \n"
           << GateTools::Indent(indent+1) << G4BestUnit( m_acceptedDistance,"Length")<<  Gateendl;
}



 double GateClustering::getDistance(G4ThreeVector pos1, G4ThreeVector pos2 ){
    double dist=std::sqrt(std::pow(pos1.getX()-pos2.getX(),2)+std::pow(pos1.getY()-pos2.getY(),2)+std::pow(pos1.getZ()-pos2.getZ(),2));
    return dist;


 }


//this is standalone only because it repeats twice in processOnePulse()
inline void GateClustering::PulsePushBack(const GatePulse* inputPulse, GatePulseList& outputPulseList)
{
	GatePulse* outputPulse = new GatePulse(*inputPulse);
    if (nVerboseLevel>1)
		G4cout << "Created new pulse for volume " << inputPulse->GetVolumeID() << ".\n"
		<< "Resulting pulse is: \n"
		<< *outputPulse << Gateendl << Gateendl ;
	outputPulseList.push_back(outputPulse);
}


