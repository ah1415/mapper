/* 
 * Copyright 2019 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <cfloat>
#include <algorithm>
#include <queue>
#include <list>
#include <chrono>
#include "nodes.h"
#include "m4.h"
#include "m3.h"
#include <tuple>
#include <map>
#include <set>
#include <random>
#include <unordered_set>
struct Path {
    // Specifies one subpath of the courier truck route

    // The intersection id where a start depot, pick-up intersection or drop-off intersection 
    // is located
    unsigned start_intersection;

    // The intersection id where this subpath ends. This must be the 
    // start_intersection of the next subpath or the intersection of an end depot
    unsigned end_intersection;

    // Street segment ids of the path between start_intersection and end_intersection 
    // They form a connected path (see m3.h)
    std::vector<unsigned> subpath;
    
    double time;
};



struct WaveElem2{
    Node *node;
    
    double travel_time;
    
    WaveElem2(Node *n, double time){
        node=n;
        travel_time=time;
        
    }
};
struct comp2{
    bool operator()(WaveElem2 e1, WaveElem2 e2){
        return e1.travel_time>e2.travel_time;
    }
    
};
struct locs{
    unsigned delid;
    unsigned id;
    bool pd;
    double weight;
};
void perturb(std::vector<locs> &order,std::unordered_map<unsigned,std::unordered_map<unsigned,Path> > &deliveryloc,double &qor, double truck_capacity, const std::vector<DeliveryInfo>& deliveries);
std::unordered_map<unsigned, Path> bfsPath(std::vector<unsigned> sourceID, std::vector<unsigned> destID, std::vector<Node> nodes_list, double right_turn_penalty, double left_turn_penalty);
Path bfsTraceBack(unsigned destID,std::vector<Node> &nodes_list, double right_turn_penalty,double left_turn_penalty);
// This routine takes in a vector of N deliveries (pickUp, dropOff
// intersection pairs), another vector of M intersections that
// are legal start and end points for the path (depots), right and left turn 
// penalties in seconds (see m3.h for details on turn penalties), 
// and the truck_capacity in pounds.
//
// The first vector 'deliveries' gives the delivery information.  Each delivery
// in this vector has pickUp and dropOff intersection ids and the weight (also
// in pounds) of the delivery item. A delivery can only be dropped-off after
// the associated item has been picked-up. 
// 
// The second vector 'depots' gives the intersection ids of courier company
// depots containing trucks; you start at any one of these depots and end at
// any one of the depots.
//
// This routine returns a vector of CourierSubpath objects that form a delivery route.
// The CourierSubpath is as defined above. The first street segment id in the
// first subpath is connected to a depot intersection, and the last street
// segment id of the last subpath also connects to a depot intersection.  The
// route must traverse all the delivery intersections in an order that allows
// all deliveries to be made with the given truck capacity. Addionally, a package
// should not be dropped off if you haven't picked it up yet.
//
// The start_intersection of each subpath in the returned vector should be 
// at least one of the following (a pick-up and/or drop-off can only happen at 
// the start_intersection of a CourierSubpath object):
//      1- A start depot.
//      2- A pick-up location (and you must specify the indices of the picked 
//                              up orders in pickup_indices)
//      3- A drop-off location. 
//
// You can assume that N is always at least one, M is always at least one
// (i.e. both input vectors are non-empty), and truck_capacity is always greater
// or equal to zero.
//
// It is legal for the same intersection to appear multiple times in the pickUp
// or dropOff list (e.g. you might have two deliveries with a pickUp
// intersection id of #50). The same intersection can also appear as both a
// pickUp location and a dropOff location.
//        
// If you have two pickUps to make at an intersection, traversing the
// intersection once is sufficient to pick up both packages, as long as the
// truck_capcity fits both of them and you properly set your pickup_indices in
// your courierSubpath.  One traversal of an intersection is sufficient to
// drop off all the (already picked up) packages that need to be dropped off at
// that intersection.
//
// Depots will never appear as pickUp or dropOff locations for deliveries.
//  
// If no valid route to make *all* the deliveries exists, this routine must
// return an empty (size == 0) vector.
auto start=std::chrono::high_resolution_clock::now();
std::vector<CourierSubpath> traveling_courier(
		const std::vector<DeliveryInfo>& deliveries,
	       	const std::vector<unsigned>& depots, 
		const float right_turn_penalty, 
		const float left_turn_penalty, 
		const float truck_capacity){
    
        start=std::chrono::high_resolution_clock::now();
        if(deliveries.size()==0||depots.size()==0)
        return {};
        
        std::vector<locs >orderp;
    std::vector<CourierSubpath> travel_path;
    std::vector<unsigned> pickups;
    //std::vector<unsigned> dropoffs;
    std::set<unsigned> all;
    for(int i=0; i<int(deliveries.size());i++){
        if(deliveries[i].itemWeight>truck_capacity){
            return {};
        }
        pickups.push_back(deliveries[i].pickUp);
        //dropoffs.push_back(deliveries[i].dropOff);
        all.insert(deliveries[i].pickUp);
        all.insert(deliveries[i].dropOff);
    }
    std::vector<unsigned>allt(all.begin(),all.end());
    allt.insert(allt.end(),depots.begin(),depots.end());
    
    std::unordered_map<unsigned,std::unordered_map<unsigned,Path> > deliveryloc;
    std::map<unsigned,std::vector<locs> >orderm;
    #pragma omp parallel for
    for(int i=0;i<int(all.size());i++){
        auto it=all.begin();
        std::advance(it,i);
        std::unordered_map<unsigned,Path>temp;
        temp=bfsPath({*it},allt,node_list,right_turn_penalty,left_turn_penalty);
        #pragma omp critical
        deliveryloc.insert({*it,temp});
    }
    
    
    
    std::unordered_map<unsigned,Path> depot;
    {
        std::vector<Node> nodes_list=node_list;
        depot=bfsPath(depots, pickups, nodes_list, right_turn_penalty, left_turn_penalty);
    }

    double best=DBL_MAX;
bool fail=false;
    #pragma omp parallel for    
        for(int y=0;y<deliveries.size();y++){
            
            double qor=0;
    unsigned curr;
    std::vector<int> pick;
    double weightn=0;
    int deli;
    std::vector<locs >order;
    {
        
        deli=y;
        curr=deliveries[deli].pickUp;
        
        order.push_back({deli,curr,true,deliveries[deli].itemWeight});

    }
    bool up=true;
    int count =1;
    
    
    std::list <int> nv;
    for(int i=0; i<deliveries.size();i++){
        nv.push_back(i);
    }
    
    while(count <2*deliveries.size()){
        
        
        
        if(up){
            
            weightn+=deliveries[deli].itemWeight;
            nv.remove(deli);
            pick.push_back(deli);
            up=false;
        }
        double t1=DBL_MAX, t2=DBL_MAX;
        unsigned i1,i2;
        int d1;
        
        for(int i=0; i<pick.size();i++){
            //DROP OFF STUFF
            if(deliveries[pick[i]].dropOff==curr){
                weightn-=deliveries[pick[i]].itemWeight;
                pick.erase(pick.begin()+i);
                i--;
            
            //else 
            }else if(deliveryloc[curr][deliveries[pick[i]].dropOff].time<t2){
                t2=deliveryloc[curr][deliveries[pick[i]].dropOff].time;
                i2=deliveries[pick[i]].dropOff;
            }
        }
        
        for(auto i =nv.begin();i!=nv.end();i++){
                 if(deliveryloc[curr][deliveries[*i].pickUp].time<t1&&(weightn+deliveries[*i].itemWeight)<=truck_capacity){
                    t1=deliveryloc[curr][deliveries[*i].pickUp].time;
                i1=deliveries[*i].pickUp;
                d1=*i;
                
            }
        }

        
        if(t1==DBL_MAX&&t2==DBL_MAX){
            fail=true;
            order={};
            qor=DBL_MAX;
            break;
        }else if(t2<=t1){
            
            qor+=deliveryloc[curr][i2].time;
            curr=i2;
            for(int i=0;i<pick.size();i++){
                if(curr==deliveries[pick[i]].dropOff){
                    count++;
                    order.push_back({pick[i],curr,false,deliveries[pick[i]].itemWeight});
                }
            }
            
            //travelp.push_back(temp);
        }else{
            
            qor+=deliveryloc[curr][i1].time;
            curr=i1;
            deli=d1;
            up=true;
            count++;
            order.push_back({deli,curr,true,deliveries[deli].itemWeight});
            //travelp.push_back(temp);
        }
        
    }
#pragma omp critical
    orderm.insert({qor,order});

        }
    if(fail)
        return {};
    //std::cout<<(std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()<<std::endl;

 
        for(int y=0;y<orderm.size();y++){
            if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()<42.0){
                std::vector<CourierSubpath> travelp;
                auto it=orderm.begin();
                std::advance(it,y);
                std::vector<locs >order=(*it).second;
                double qor=(*it).first;
                
                if(order.size()>0){
             perturb(order,deliveryloc,qor, truck_capacity, deliveries);
             
    //2OPT
    /*
    for(int i=1;i<order.size()-1;i++){
        if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()>40.0)
                break;
        for(int j=i+1;j<order.size();j++){
            //auto it1r=order.rend()-i;
            //auto it2r=order.rend()-j;
                auto it1=order.begin()+i;
            auto it2=order.begin()+j;
            for(int m=0;m<5;m++){
                std::vector<std::tuple<int,bool,double> >ordernew;
                if(m==0){
                    ordernew.insert(ordernew.end(),order.begin(),it1);
                ordernew.insert(ordernew.end(),it2,order.end());
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==1){
                    ordernew.insert(ordernew.end(),it2,order.end());
                ordernew.insert(ordernew.end(),order.begin(),it1);
            
            ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==2){
                    ordernew.insert(ordernew.end(),it1,it2);
            ordernew.insert(ordernew.end(),order.begin(),it1);
            ordernew.insert(ordernew.end(),it2,order.end());
                }else if(m==3){
                    ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it2,order.end());
                ordernew.insert(ordernew.end(),order.begin(),it1);
                }else if(m==4){
                    ordernew.insert(ordernew.end(),it2,order.end());
               
            ordernew.insert(ordernew.end(),it1,it2);
             ordernew.insert(ordernew.end(),order.begin(),it1);
                }
                
            double qor2=0;
                double weightn2=0;
                bool faile=false;
                std::vector<unsigned>pick2;
                for(int k=0;k<ordernew.size()-1;k++){
                    int start, end;
                    if(std::get<1>(ordernew[k])){
                        start=deliveries[std::get<0>(ordernew[k])].pickUp;
                        weightn2+=std::get<2>(ordernew[k]);
                        if(weightn2>truck_capacity){
                            faile=true;
                            break;
                        }
                        pick2.push_back(std::get<0>(ordernew[k]));
                    }else{
                        start=deliveries[std::get<0>(ordernew[k])].dropOff;
                        weightn2-=std::get<2>(ordernew[k]);
                        faile=true;
                        for(int l=0;l<pick2.size();l++){
                       
                            if(pick2[l]==std::get<0>(ordernew[k])){
                                pick2.erase(pick2.begin()+l);
                                faile=false;
                                break;
                            }
                        }
                        if(faile){
                            break;
                        }
                    }
                    if(std::get<1>(ordernew[k+1])){
                        end=deliveries[std::get<0>(ordernew[k+1])].pickUp;
                    }else{
                        end=deliveries[std::get<0>(ordernew[k+1])].dropOff;
                    }
                    qor2+=deliveryloc[start][end].time;
                    
                }
                if(!faile){
                    if(qor2<qor){
                        qor=qor2;
                        order=ordernew;
                        i=0;
                        goto brk;
                        //std::cout<<"Found"<<std::endl;
                        
                    }
                }
            }
            
            
        }
        brk: ;
    }
             *//*
#pragma omp parallel for
             
             for(int p=0;p<8;p++){
                 double qb=qor;
                 std::vector<locs> ob=order;
                 double temp=1;
                 do{
             double q=qb;
             std::vector<locs> o=ob;
             std::default_random_engine generator((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()+p+y+5);
             std::uniform_int_distribution<int> distribution(1,order.size()-3);
    for(int i=1;i<order.size()-2;i++){
        int a=distribution(generator);
        for(int j=a+1;j<order.size()-1;j++){
     for(int n=j+1;n<order.size();n++){
         if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()>44.0){
                goto stop;
        }
            //auto it1r=order.rend()-i;
            //auto it2r=order.rend()-j;
               auto it0=o.begin();
                auto it1=o.begin()+a;
                auto it2=o.begin()+j;
                auto it3 = o.begin()+n;
                auto it4=o.end();
            for(int m=22;m>=0;m--){
                std::vector<locs >ordernew;
                if(m==0){
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==1){
                    ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it3,it4);
                }else if(m==2){
                    ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==3){
                    ordernew.insert(ordernew.end(),it0,it1);
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                
                }else if(m==4){
                    ordernew.insert(ordernew.end(),it0,it1);
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==5){
                    ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==6){
                    ordernew.insert(ordernew.end(),it1,it2);
                    ordernew.insert(ordernew.end(),it0,it1);
                    
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it3,it4);
                
                }else if(m==7){
                    ordernew.insert(ordernew.end(),it1,it2);
                    
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it3,it4);
                }else if(m==8){
                    ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                }else if(m==9){
                    ordernew.insert(ordernew.end(),it1,it2);
                    
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it0,it1);
                
                }else if(m==17){
                    ordernew.insert(ordernew.end(),it2,it3);
                    ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==18){
                    ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it0,it1);
                
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==19){
                    ordernew.insert(ordernew.end(),it2,it3);
                    ordernew.insert(ordernew.end(),it0,it1);
                    
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it3,it4);
                
                }else if(m==20){
                    ordernew.insert(ordernew.end(),it2,it3);
                    
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it3,it4);
                }else if(m==21){
                    ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                
                ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                }else if(m==22){
                    ordernew.insert(ordernew.end(),it2,it3);
                    
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                
                }else if(m==10){
                    ordernew.insert(ordernew.end(),it2,it3);
                    ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==11){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==12){
                     ordernew.insert(ordernew.end(),it3,it4);
                    
                    ordernew.insert(ordernew.end(),it0,it1);
                    
                ordernew.insert(ordernew.end(),it1,it2);
               ordernew.insert(ordernew.end(),it2,it3);
                
                }else if(m==13){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==14){
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it0,it1);
                }else if(m==15){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                    ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                
                }else if(m==16){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                   ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                }
                
            double qor2=0;
                double weightn2=0;
                bool faile=false;
                std::vector<unsigned>pick2;
                for(int k=0;k<ordernew.size()-1;k++){
                    int starte=ordernew[k].id, ende=ordernew[k+1].id;
                    if(ordernew[k].pd){
                        
                        weightn2+=ordernew[k].weight;
                        if(weightn2>truck_capacity){
                            faile=true;
                            break;
                        }
                        pick2.push_back(ordernew[k].delid);
                    }else{
                        
                        weightn2-=ordernew[k].weight;
                        faile=true;
                        for(int l=0;l<pick2.size();l++){
                       
                            if(pick2[l]==ordernew[k].delid){
                                pick2.erase(pick2.begin()+l);
                                faile=false;
                                break;
                            }
                        }
                        if(faile){
                            break;
                        }
                    }
                    
                    qor2+=deliveryloc[starte][ende].time;
                    
                }
                if(!faile){
                    
                        q=qor2;
                        o=ordernew;
                        
                        goto bk;
                        //std::cout<<"Found"<<std::endl;
                        
                    
                }
            }
            
     }
        }
        
    }
             bk: ;
#pragma omp critical
             {
                 if(q<qor){
                     qor=q;
                     order=o;
                 }
             }
             double delta=q-qb;
             if(q<qb||distribution(generator)%2<exp(-delta/temp)){
                 qb=q;
                 ob=o;
             }
             temp*=0.999;
            // std::cout<<temp<<std::endl;
                 }while(order.size()>10&&temp>0.001);
    stop: ;
    
             }
             
       */              
#pragma omp parallel for schedule(dynamic) num_threads(8)
             
             for(int p=0;p<16;p++){
             double q=qor;
             std::vector<locs> o=order;
             std::default_random_engine generator(100*p+5*y+31);
             std::uniform_int_distribution<int> distribution(1,order.size()-3);
    for(int i=1;i<order.size()-2;i++){
        int a=distribution(generator);
        for(int j=a+1;j<order.size()-1;j++){
     for(int n=j+1;n<order.size();n++){
         if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()>44.0){
                goto stop2;
        }
            //auto it1r=order.rend()-i;
            //auto it2r=order.rend()-j;
               auto it0=o.begin();
                auto it1=o.begin()+a;
                auto it2=o.begin()+j;
                auto it3 = o.begin()+n;
                auto it4=o.end();
            for(int m=22;m>=0;m--){
                std::vector<locs >ordernew;
                if(m==0){
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==1){
                    ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it3,it4);
                }else if(m==2){
                    ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==3){
                    ordernew.insert(ordernew.end(),it0,it1);
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                
                }else if(m==4){
                    ordernew.insert(ordernew.end(),it0,it1);
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==5){
                    ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==6){
                    ordernew.insert(ordernew.end(),it1,it2);
                    ordernew.insert(ordernew.end(),it0,it1);
                    
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it3,it4);
                
                }else if(m==7){
                    ordernew.insert(ordernew.end(),it1,it2);
                    
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it3,it4);
                }else if(m==8){
                    ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                }else if(m==9){
                    ordernew.insert(ordernew.end(),it1,it2);
                    
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it0,it1);
                
                }else if(m==17){
                    ordernew.insert(ordernew.end(),it2,it3);
                    ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==18){
                    ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it0,it1);
                
                ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==19){
                    ordernew.insert(ordernew.end(),it2,it3);
                    ordernew.insert(ordernew.end(),it0,it1);
                    
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it3,it4);
                
                }else if(m==20){
                    ordernew.insert(ordernew.end(),it2,it3);
                    
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it3,it4);
                }else if(m==21){
                    ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                
                ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                }else if(m==22){
                    ordernew.insert(ordernew.end(),it2,it3);
                    
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                
                }else if(m==10){
                    ordernew.insert(ordernew.end(),it2,it3);
                    ordernew.insert(ordernew.end(),it3,it4);
                
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==11){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it1,it2);
                }else if(m==12){
                     ordernew.insert(ordernew.end(),it3,it4);
                    
                    ordernew.insert(ordernew.end(),it0,it1);
                    
                ordernew.insert(ordernew.end(),it1,it2);
               ordernew.insert(ordernew.end(),it2,it3);
                
                }else if(m==13){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it2,it3);
                }else if(m==14){
                    ordernew.insert(ordernew.end(),it3,it4);
                ordernew.insert(ordernew.end(),it1,it2);
                
                ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it0,it1);
                }else if(m==15){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                    ordernew.insert(ordernew.end(),it2,it3);
                ordernew.insert(ordernew.end(),it1,it2);
                ordernew.insert(ordernew.end(),it0,it1);
                
                }else if(m==16){
                    ordernew.insert(ordernew.end(),it3,it4);
                    
                   ordernew.insert(ordernew.end(),it2,it3);
                
                ordernew.insert(ordernew.end(),it0,it1);
                ordernew.insert(ordernew.end(),it1,it2);
                }
                
            double qor2=0;
                double weightn2=0;
                bool faile=false;
                std::vector<unsigned>pick2;
                for(int k=0;k<ordernew.size()-1;k++){
                    int starte=ordernew[k].id, ende=ordernew[k+1].id;
                    if(ordernew[k].pd){
                        
                        weightn2+=ordernew[k].weight;
                        if(weightn2>truck_capacity){
                            faile=true;
                            break;
                        }
                        pick2.push_back(ordernew[k].delid);
                    }else{
                        
                        weightn2-=ordernew[k].weight;
                        faile=true;
                        for(int l=0;l<pick2.size();l++){
                       
                            if(pick2[l]==ordernew[k].delid){
                                pick2.erase(pick2.begin()+l);
                                faile=false;
                                break;
                            }
                        }
                        if(faile){
                            break;
                        }
                    }
                    
                    qor2+=deliveryloc[starte][ende].time;
                    
                }
                if(!faile){
                    if(qor2<q){
                        q=qor2;
                        o=ordernew;
                        i=0;
                        goto brk;
                        //std::cout<<"Found"<<std::endl;
                        
                    }
                }
            }
            
     }
        }
        brk: ;
    }
             stop2: ;
             #pragma omp critical 
    {
        if(q<qor){
            qor=q;
            order=o;
        }
    }
             }
        perturb(order,deliveryloc,qor, truck_capacity, deliveries);
    
    
    
    
    {   
      
            qor+=depot[order[0].id].time;
            double curr=order[order.size()-1].id;
           int index=0;
        
            double maxtime=DBL_MAX;
            for(int i=0;i<int(depots.size());i++){
                double time;
                std::vector<unsigned> path=deliveryloc[curr][depots[i]].subpath;
                if(path.size()!=0){
                    time=deliveryloc[curr][depots[i]].time;
                if (time<maxtime){
                    maxtime=time;
                    index=i;
                    
                }}
            }
            
        
        

        qor+=deliveryloc[curr][depots[index]].time;
 
       }    

    {
        if(qor<best){
            best=qor;
            orderp=order;
            
        }
    }
    
                }
            }
        }
    
    {
            CourierSubpath temp;
            temp.end_intersection=orderp[0].id;
            temp.start_intersection=depot[temp.end_intersection].start_intersection;
            temp.subpath=depot[temp.end_intersection].subpath;
            if(temp.subpath.size()==0&&temp.end_intersection!=temp.start_intersection){
                return {};
            }
            travel_path.push_back(temp);
            
        }
        for(int i=0; i<orderp.size()-1;i++){
            CourierSubpath temp;
            temp.start_intersection=orderp[i].id;
            if(orderp[i].pd){
                temp.pickUp_indices={orderp[i].delid};
            }
            
            temp.end_intersection=orderp[i+1].id;
            temp.subpath=deliveryloc[temp.start_intersection][temp.end_intersection].subpath;
            travel_path.push_back(temp);
        }
        double curr=orderp[orderp.size()-1].id;
        
        CourierSubpath temp;
        
        
        int index=0;
        {
            double maxtime=DBL_MAX;
            for(int i=0;i<int(depots.size());i++){
                double time;
                std::vector<unsigned> path=deliveryloc[curr][depots[i]].subpath;
                if(path.size()!=0){
                    time=deliveryloc[curr][depots[i]].time;
                if (time<maxtime){
                    maxtime=time;
                    index=i;
                    
                }}
            }
            
        }
       
       
        temp.start_intersection=curr;
        temp.end_intersection=depots[index];
        temp.subpath=deliveryloc[curr][temp.end_intersection].subpath;
        travel_path.push_back(temp);
          
    return travel_path;
}
void perturb(std::vector<locs >&order,std::unordered_map<unsigned,std::unordered_map<unsigned,Path> > &deliveryloc,double &qor, double truck_capacity, const std::vector<DeliveryInfo>& deliveries){
    
    /*
    for(int i=0;i<order.size()-1;i++){
        
        for(int j=i+1;j<order.size();j++){
            if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()>40.0)
                return;
            std::vector<std::tuple<int,bool,double> >orders=order;
            
                if(std::get<0>(orders[i])==std::get<0>(orders[j])){
                    break;
                }
                {
                    std::tuple<int,bool,double> temp=orders[i];
                    orders[i]=orders[j];
                    orders[j]=temp;
                }
                
                double qor2=0;
                double weightn2=0;
                bool faile=false;
                std::vector<unsigned>pick2;
                for(int k=0;k<orders.size()-1;k++){
                    if(i<k&&std::get<0>(orders[k])==std::get<0>(orders[i])&&!std::get<1>(orders[i])){
                        faile=true;break;
                    }else if(i>k&&std::get<0>(orders[k])==std::get<0>(orders[i])&&std::get<1>(orders[i])){
                        faile=true;break;
                    }
                    int starte, ende;
                    if(std::get<1>(orders[k])){
                        starte=deliveries[std::get<0>(orders[k])].pickUp;
                        weightn2+=std::get<2>(orders[k]);
                        if(weightn2>truck_capacity){
                            faile=true;
                            break;
                        }
                        
                    }else{
                        starte=deliveries[std::get<0>(orders[k])].dropOff;
                        weightn2-=std::get<2>(orders[k]);
                        
                    }
                    if(std::get<1>(orders[k+1])){
                        ende=deliveries[std::get<0>(orders[k+1])].pickUp;
                    }else{
                        ende=deliveries[std::get<0>(orders[k+1])].dropOff;
                    }
                    qor2+=deliveryloc[starte][ende].time;
                    
                }
                if(!faile){
                    if(qor2<qor){
                        qor=qor2;
                        order=orders;
                        
                        //if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()<40.0)
                        //rep=true;
                    }
                }
                
            
            
        }
    }
    */
    for(int i=0;i<order.size();i++){
        bool found=false;
        for(int j=0;j<order.size();j++){
            if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()>44.5)
                return;
            std::vector<locs >orders=order;
            if(orders[i].pd){
                if(orders[i].delid==orders[j].delid&&i!=j){
                    break;
                }
                {
                    locs temp=orders[i];
                    orders[i]=orders[j];
                    orders[j]=temp;
                }
                
                double qor2=0;
                double weightn2=0;
                bool faile=false;
                std::vector<unsigned>pick2;
                for(int k=0;k<orders.size()-1;k++){
                    if(i<k&&orders[k].delid==orders[i].delid&&!orders[i].pd){
                        faile=true;break;
                    }else if(i>k&&orders[k].delid==orders[i].delid&&orders[i].pd){
                        faile=true;break;
                    }else if(j<k&&orders[k].delid==orders[j].delid&&!orders[j].pd){
                        faile=true;break;
                    }else if(j>k&&orders[k].delid==orders[j].delid&&orders[j].pd){
                        faile=true;break;
                    }
                    int starte=orders[k].id, ende=orders[k+1].id;
                    if(orders[k].pd){
                        
                        weightn2+=orders[k].weight;
                        if(weightn2>truck_capacity){
                            faile=true;
                            break;
                        }
                        
                    }else{
                        
                        weightn2-=orders[k].weight;
                        
                    }
                    
                    qor2+=deliveryloc[starte][ende].time;
                    
                }
                if(!faile){
                    if(qor2<qor){
                        qor=qor2;
                        order=orders;
                        //if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()<38.0)
                        //rep=true;
                    }
                }
                
            }else{
                if(found){
                {
                    locs temp=orders[i];
                    orders[i]=orders[j];
                    orders[j]=temp;
                }
                
                double qor2=0;
                double weightn2=0;
                bool faile=false;
                
                for(int k=0;k<orders.size()-1;k++){
                    if(i<k&&orders[k].delid==orders[i].delid&&!orders[i].pd){
                        faile=true;break;
                    }else if(i>k&&orders[k].delid==orders[i].delid&&orders[i].pd){
                        faile=true;break;
                    }else if(j<k&&orders[k].delid==orders[j].delid&&!orders[j].pd){
                        faile=true;break;
                    }else if(j>k&&orders[k].delid==orders[j].delid&&orders[j].pd){
                        faile=true;break;
                    }
                    int starte=orders[k].id, ende=orders[k+1].id;
                    if(orders[k].pd){
                        
                        weightn2+=orders[k].weight;
                        if(weightn2>truck_capacity){
                            faile=true;
                            break;
                        }
                        
                    }else{
                        
                        weightn2-=orders[k].weight;
                        
                    }
                    
                    qor2+=deliveryloc[starte][ende].time;
                    
                }
                if(!faile){
                    if(qor2<qor){
                        qor=qor2;
                        order=orders;
                        //if((std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::high_resolution_clock::now()-start)).count()<38.0)
                        //rep=true;
                    }
                }
                }else if(orders[i].delid==orders[j].delid&&i!=j){
                    found=true;
                }
            }
        }
    }

}

std::unordered_map<unsigned, Path> bfsPath(std::vector<unsigned> sourceID, std::vector<unsigned> destID, std::vector<Node> nodes_list, double right_turn_penalty, double left_turn_penalty){
    
    //Create min heap priority queue of WaveElem and insert source node
    std::priority_queue<WaveElem2, std::vector<WaveElem2>, comp2> wavefront;
    
    
    for(int i=0; i<sourceID.size();i++){
        Node * source_node = &nodes_list[sourceID[i]];
        source_node->best_time=0;
        wavefront.push(WaveElem2(source_node,0));
        

    }
    int count =0;
    //Continue searching until destination is reached or wavefront is empty(not found)
    while(wavefront.size()>0){
        
        //Remove minimum element
        WaveElem2 wave = wavefront.top();
        wavefront.pop();
        Node *curr_node = wave.node;
        //Check if path has improved travel time and only re-expand if it is
        if(wave.travel_time== curr_node->best_time){
            
            //If destination found exit loop
            count+=std::count(destID.begin(),destID.end(),curr_node->id);
            if(count==destID.size()){
                break;
            }
            //Insert connected nodes from current node into wavefront
            for(int i=0; i<int(curr_node->out_edge.size());i++){
                
                //Check to not go backwards over reaching edge (wasted insertion)
                if(curr_node->reaching_edge!=int(curr_node->out_edge[i])){
                    
                    //Get connected node and update distance if not computed
                    Node *to_node=&nodes_list[curr_node->inter[i]];
                    
                    //Determine total time spent to get to next node
                    double time = curr_node->best_time+curr_node->time[i];
                    if(curr_node->reaching_edge!=NO_EDGE){
                        TurnType turn = find_turn_type(curr_node->reaching_edge,curr_node->out_edge[i]);
                        if(turn ==TurnType::RIGHT){
                            time +=right_turn_penalty;

                        }else if(turn ==TurnType::LEFT){
                            time +=left_turn_penalty;
                        }
                    }
                    if(time<to_node->best_time){
                        to_node->best_time=time;
                        to_node->reaching_edge=curr_node->out_edge[i];


                        //Insert next node into wavefront
                        wavefront.push(WaveElem2(to_node, time));
                    }
                }
            }
        }
    }
    std::unordered_map<unsigned,Path> paths;
    for(int i=0;i<destID.size();i++){
        paths.insert({destID[i],bfsTraceBack(destID[i], nodes_list, right_turn_penalty,left_turn_penalty)});
    }
    return paths;
}


Path bfsTraceBack(unsigned destID, std::vector<Node> &nodes_list, double right_turn_penalty,double left_turn_penalty){
    Path path;
    path.end_intersection=destID;
    Node*curr_node = &nodes_list[destID];
    
    //Move through reaching edges from destination to source and store in path vector
    while(curr_node->reaching_edge!=NO_EDGE){
        
        path.subpath.push_back(curr_node->reaching_edge);
        
        //Determine which node is connected to current node in traceback
        if(unsigned(getInfoStreetSegment(path.subpath.back()).from)==curr_node->id){
            curr_node=&nodes_list[getInfoStreetSegment(path.subpath.back()).to];
        }else{
            curr_node=&nodes_list[getInfoStreetSegment(path.subpath.back()).from];
        }
        
    }
    path.start_intersection=curr_node->id;
    //Reverse path to get from start to end
    std::reverse(path.subpath.begin(), path.subpath.end());
    path.time=compute_path_travel_time(path.subpath,right_turn_penalty,left_turn_penalty);
    return path;
}