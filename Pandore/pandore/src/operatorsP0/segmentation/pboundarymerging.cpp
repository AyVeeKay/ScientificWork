/* -*- mode: c++; c-basic-offset: 3 -*-
 *
 * Copyright (c), GREYC.
 * All rights reserved
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the GREYC, nor the name of its
 *     contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 *
 * 
 * For more information, refer to:
 * https://clouard.users.greyc.fr/Pandore
 */

/**
 * @author R�gis Clouard - 2002-07-24
 * @author Regis Clouard - 2002-12-12 (GarbageCollector)
 */

#include <pandore.h>
using namespace pandore;

/**
 * @file pboundarymerging.cpp
 * @brief  Fusion de regions en utilisant le contraste aux frontieres:
 * calcule par:
 * contraste(R1,R2)= sum(max(C(s,t)))/N
 * avec C(s,t)= |ims[s]-ims[t]|
 * t i V(s), t in R2 et s in R1.
 */

struct Arete {
   Ulong ex1;
   Ulong ex2;
};


struct Region {
   Ulong no;
   Ulong no_node;
};

/*
 * A MODIFIER SELON LE CRITERE
 */

double valuation( double sum, double nbr ) {
   if (nbr == 0) return (double)MAXULONG;
   return (double)sum / nbr;
}

/*
 * Initialisation de la lut
 */
void initialisation( Region *lut, Ulong nb_lut ) {
   for (Ulong a=0; a < nb_lut; a++)
      lut[a].no=lut[a].no_node=a;
}

/*
 * Modification de la lut
 */
void modification( Region * /*lut*/, Ulong /*R12*/, Ulong /*R1*/, Ulong /*R2*/ ) {
}

/*
 *    INVARIANT SELON LE CRITERE
 */


/*
 * Permet d'eliminer du tas, toutes les aretes
 * entre deux regions qui sont maintenant fusionnees
 * de maniere a recuprer de la place.
 */
void GarbageCollector( Heap<Arete,float,int> &tas, Region *lut ) {
   Arete* tab = new Arete[tas.Nrank()];
   float* cle = new float[tas.Nrank()];
   Ulong i=0;
   while(!tas.Empty()) {
      tab[i]=tas.Pop(&cle[i]);
      Ulong R1=tab[i].ex1;
      Ulong R2=tab[i].ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if (!((lut[R1].no!=R1) || (lut[R2].no!=R2))) {
	 i++;
      }
   }

   for (Ulong j=0; j<i;j++) {
      tas.Push(tab[j],cle[j]);
   }
   delete tab;
}


/**
 *Remplissage de la lut
 */
void remplissage( const Reg2d &rgs, const Img2duc &ims, Graph2d &gs, Graph2d &gn ) {
   Point2d p;
   Ulong r1,r2;
   GEdge *ptr_s, *ptr_n;
  
   // Initialisation
   for (int i=0; i<gs.Size(); i++)
      if ((gs[i]))
	 for (ptr_s=gs[i]->Neighbours(), ptr_n=gn[i]->Neighbours(); ptr_n!=NULL; ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next())
	    ptr_s->weight= ptr_n->weight=0.0F;

   for (p.y = 1; p.y < ims.Height() - 1; p.y++)
   for (p.x = 1; p.x < ims.Width() - 1; p.x++)
   {
      if (rgs[p] == 0) continue;
    
      for (int v=0; v<8; v++) {
	 r1=rgs[p];
	 r2=rgs[p+v8[v]];
	 if (r2 && (r1 != r2)) {
	    gn.Link(r1,r2,1.0F,true);
	    gs.Link(r1,r2,(float)ABS(ims[p]-ims[p+v8[v]]),true);
	 }
      }
   }
}

/**
 *  Code invariant par rapport au critere
 */
Errc PBoundaryMerging( const Reg2d &rgs, const Graph2d &gs, const Img2duc &ims, Reg2d &rgd, Graph2d &gd, Long number, float threshold ) {
 Region *lut;
   Ulong *p_rgd = rgd.Vector();
   Ulong *p_rgs = rgs.Vector();
   GEdge *ptr_s, *ptr_n;
   double min_0=0;
   Arete tmp;
   Ulong nb_lut=(rgs.Labels()+1);
   Graph2d gi;
   int i;
   Heap <Arete,float,int> tas(26*gs.Size());

   // supprimer le controle du nombre d'iterations
   if (number<0) 
      number=MAXLONG;
  
   gi=gs;
   gd=gs;

   // 1ere partie : Initialisation.
   lut = new Region [2*nb_lut-1];
   initialisation(lut,2*nb_lut-1);
   remplissage(rgs,ims,gd,gi);
   
   // on calcule la valuation des aretes, si valuation<=threshold alors on empile
   for (i=1;i<gs.Size();i++) {
      // si le noeud n'existe pas, on passe
      if (gd[i]==NULL) {
	 continue;
      }
      for (ptr_s=gd[i]->Neighbours(), ptr_n=gi[i]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(),ptr_n=ptr_n->Next()) {
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=i;
	    tmp.ex2=ptr_s->Node();
	    tas.Push(tmp,(float) (min_0));
	 }
      }
   } // fin du parcours des noeuds du graphe
   
   // 2eme partie : fusion 
   Long nbre=0;
   while (nbre<number && !tas.Empty()) {
      // On fusionne l'arc minimum du graphe precedent
      // s'il repond au critere        
      tmp=tas.Pop();
      Ulong R1=tmp.ex1;
      Ulong R2=tmp.ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if ((lut[R1].no!=R1) || (lut[R2].no!=R2)) continue;
      
      // on ne conserve que le sommet de numero minimum
      Ulong G1=MIN(lut[R1].no_node,lut[R2].no_node);
      Ulong G2=MAX(lut[R1].no_node,lut[R2].no_node);
      
      // on incremente le nombre de regions fusionnees
      // on met a jour la lut.
      Ulong R12=nb_lut+ ++nbre;
      modification(lut,R12,R1,R2);
      lut[R12].no_node=G1;
      lut[R12].no=R12;
      lut[R2].no_node=G1;
      lut[G1].no=R12;
      lut[R1].no=R12;
      lut[R2].no=R12;
      
      // on met le graphe a jour
      gd[G1]->seed=(gd[G1]->seed+gd[G2]->seed)/2;
      gd.Merge(G1,G2); // G1 reste, G2 disparait.
      gi.Merge(G1,G2); // G1 reste, G2 disparait.
      for (ptr_s=gd[G1]->Neighbours(),ptr_n=gi[G1]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next()) {
	 // on met le tas a jour
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=R12;
	    tmp.ex2=lut[ptr_s->Node()].no;
	    tas.Push(tmp,(float) (min_0));
	    if (tas.Full()) GarbageCollector(tas,lut);
	 }
      }
   } // fin du while
   
   // 3eme partie : reetiquetage des regions 

   // Minimisation des etiquettes.
   for (i=1;i<(int)(nb_lut+nbre);i++) {
      Ulong j=i;
      while (lut[j].no!=j) { j=lut[j].no;} 
      lut[i].no=lut[j].no_node;
   }
   // fusion dans la carte de region
   for (unsigned int f=0; f < rgd.VectorSize(); f++) {
      // pour la region qui correspond au voisin qui a fusionne, on donne le Label du 1er sommet
      p_rgd[f]=(Ulong)lut[p_rgs[f]].no;
   }
   
   gi.Delete();
   rgd.Labels(rgs.Labels());
   
   delete [] lut;
   
   return nbre; // retourne le nombre de regions fusionnees
}


/**
 *Remplissage de la lut
 */
void remplissage( const Reg2d &rgs, const Img2dsl &ims, Graph2d &gs, Graph2d &gn ) {
   Point2d p;
   Ulong r1,r2;
   GEdge *ptr_s, *ptr_n;
  
   // Initialisation
   for (int i=0; i<gs.Size(); i++)
      if ((gs[i]))
	 for (ptr_s=gs[i]->Neighbours(), ptr_n=gn[i]->Neighbours(); ptr_n!=NULL; ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next())
	    ptr_s->weight= ptr_n->weight=0.0F;

   for (p.y = 1; p.y < ims.Height() - 1; p.y++)
   for (p.x = 1; p.x < ims.Width() - 1; p.x++)
   {
      if (rgs[p] == 0) continue;
    
      for (int v=0; v<8; v++) {
	 r1=rgs[p];
	 r2=rgs[p+v8[v]];
	 if (r2 && (r1 != r2)) {
	    gn.Link(r1,r2,1.0F,true);
	    gs.Link(r1,r2,(float)ABS(ims[p]-ims[p+v8[v]]),true);
	 }
      }
   }
}

/**
 *  Code invariant par rapport au critere
 */
Errc PBoundaryMerging( const Reg2d &rgs, const Graph2d &gs, const Img2dsl &ims, Reg2d &rgd, Graph2d &gd, Long number, float threshold ) {
 Region *lut;
   Ulong *p_rgd = rgd.Vector();
   Ulong *p_rgs = rgs.Vector();
   GEdge *ptr_s, *ptr_n;
   double min_0=0;
   Arete tmp;
   Ulong nb_lut=(rgs.Labels()+1);
   Graph2d gi;
   int i;
   Heap <Arete,float,int> tas(26*gs.Size());

   // supprimer le controle du nombre d'iterations
   if (number<0) 
      number=MAXLONG;
  
   gi=gs;
   gd=gs;

   // 1ere partie : Initialisation.
   lut = new Region [2*nb_lut-1];
   initialisation(lut,2*nb_lut-1);
   remplissage(rgs,ims,gd,gi);
   
   // on calcule la valuation des aretes, si valuation<=threshold alors on empile
   for (i=1;i<gs.Size();i++) {
      // si le noeud n'existe pas, on passe
      if (gd[i]==NULL) {
	 continue;
      }
      for (ptr_s=gd[i]->Neighbours(), ptr_n=gi[i]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(),ptr_n=ptr_n->Next()) {
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=i;
	    tmp.ex2=ptr_s->Node();
	    tas.Push(tmp,(float) (min_0));
	 }
      }
   } // fin du parcours des noeuds du graphe
   
   // 2eme partie : fusion 
   Long nbre=0;
   while (nbre<number && !tas.Empty()) {
      // On fusionne l'arc minimum du graphe precedent
      // s'il repond au critere        
      tmp=tas.Pop();
      Ulong R1=tmp.ex1;
      Ulong R2=tmp.ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if ((lut[R1].no!=R1) || (lut[R2].no!=R2)) continue;
      
      // on ne conserve que le sommet de numero minimum
      Ulong G1=MIN(lut[R1].no_node,lut[R2].no_node);
      Ulong G2=MAX(lut[R1].no_node,lut[R2].no_node);
      
      // on incremente le nombre de regions fusionnees
      // on met a jour la lut.
      Ulong R12=nb_lut+ ++nbre;
      modification(lut,R12,R1,R2);
      lut[R12].no_node=G1;
      lut[R12].no=R12;
      lut[R2].no_node=G1;
      lut[G1].no=R12;
      lut[R1].no=R12;
      lut[R2].no=R12;
      
      // on met le graphe a jour
      gd[G1]->seed=(gd[G1]->seed+gd[G2]->seed)/2;
      gd.Merge(G1,G2); // G1 reste, G2 disparait.
      gi.Merge(G1,G2); // G1 reste, G2 disparait.
      for (ptr_s=gd[G1]->Neighbours(),ptr_n=gi[G1]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next()) {
	 // on met le tas a jour
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=R12;
	    tmp.ex2=lut[ptr_s->Node()].no;
	    tas.Push(tmp,(float) (min_0));
	    if (tas.Full()) GarbageCollector(tas,lut);
	 }
      }
   } // fin du while
   
   // 3eme partie : reetiquetage des regions 

   // Minimisation des etiquettes.
   for (i=1;i<(int)(nb_lut+nbre);i++) {
      Ulong j=i;
      while (lut[j].no!=j) { j=lut[j].no;} 
      lut[i].no=lut[j].no_node;
   }
   // fusion dans la carte de region
   for (unsigned int f=0; f < rgd.VectorSize(); f++) {
      // pour la region qui correspond au voisin qui a fusionne, on donne le Label du 1er sommet
      p_rgd[f]=(Ulong)lut[p_rgs[f]].no;
   }
   
   gi.Delete();
   rgd.Labels(rgs.Labels());
   
   delete [] lut;
   
   return nbre; // retourne le nombre de regions fusionnees
}


/**
 *Remplissage de la lut
 */
void remplissage( const Reg2d &rgs, const Img2dsf &ims, Graph2d &gs, Graph2d &gn ) {
   Point2d p;
   Ulong r1,r2;
   GEdge *ptr_s, *ptr_n;
  
   // Initialisation
   for (int i=0; i<gs.Size(); i++)
      if ((gs[i]))
	 for (ptr_s=gs[i]->Neighbours(), ptr_n=gn[i]->Neighbours(); ptr_n!=NULL; ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next())
	    ptr_s->weight= ptr_n->weight=0.0F;

   for (p.y = 1; p.y < ims.Height() - 1; p.y++)
   for (p.x = 1; p.x < ims.Width() - 1; p.x++)
   {
      if (rgs[p] == 0) continue;
    
      for (int v=0; v<8; v++) {
	 r1=rgs[p];
	 r2=rgs[p+v8[v]];
	 if (r2 && (r1 != r2)) {
	    gn.Link(r1,r2,1.0F,true);
	    gs.Link(r1,r2,(float)ABS(ims[p]-ims[p+v8[v]]),true);
	 }
      }
   }
}

/**
 *  Code invariant par rapport au critere
 */
Errc PBoundaryMerging( const Reg2d &rgs, const Graph2d &gs, const Img2dsf &ims, Reg2d &rgd, Graph2d &gd, Long number, float threshold ) {
 Region *lut;
   Ulong *p_rgd = rgd.Vector();
   Ulong *p_rgs = rgs.Vector();
   GEdge *ptr_s, *ptr_n;
   double min_0=0;
   Arete tmp;
   Ulong nb_lut=(rgs.Labels()+1);
   Graph2d gi;
   int i;
   Heap <Arete,float,int> tas(26*gs.Size());

   // supprimer le controle du nombre d'iterations
   if (number<0) 
      number=MAXLONG;
  
   gi=gs;
   gd=gs;

   // 1ere partie : Initialisation.
   lut = new Region [2*nb_lut-1];
   initialisation(lut,2*nb_lut-1);
   remplissage(rgs,ims,gd,gi);
   
   // on calcule la valuation des aretes, si valuation<=threshold alors on empile
   for (i=1;i<gs.Size();i++) {
      // si le noeud n'existe pas, on passe
      if (gd[i]==NULL) {
	 continue;
      }
      for (ptr_s=gd[i]->Neighbours(), ptr_n=gi[i]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(),ptr_n=ptr_n->Next()) {
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=i;
	    tmp.ex2=ptr_s->Node();
	    tas.Push(tmp,(float) (min_0));
	 }
      }
   } // fin du parcours des noeuds du graphe
   
   // 2eme partie : fusion 
   Long nbre=0;
   while (nbre<number && !tas.Empty()) {
      // On fusionne l'arc minimum du graphe precedent
      // s'il repond au critere        
      tmp=tas.Pop();
      Ulong R1=tmp.ex1;
      Ulong R2=tmp.ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if ((lut[R1].no!=R1) || (lut[R2].no!=R2)) continue;
      
      // on ne conserve que le sommet de numero minimum
      Ulong G1=MIN(lut[R1].no_node,lut[R2].no_node);
      Ulong G2=MAX(lut[R1].no_node,lut[R2].no_node);
      
      // on incremente le nombre de regions fusionnees
      // on met a jour la lut.
      Ulong R12=nb_lut+ ++nbre;
      modification(lut,R12,R1,R2);
      lut[R12].no_node=G1;
      lut[R12].no=R12;
      lut[R2].no_node=G1;
      lut[G1].no=R12;
      lut[R1].no=R12;
      lut[R2].no=R12;
      
      // on met le graphe a jour
      gd[G1]->seed=(gd[G1]->seed+gd[G2]->seed)/2;
      gd.Merge(G1,G2); // G1 reste, G2 disparait.
      gi.Merge(G1,G2); // G1 reste, G2 disparait.
      for (ptr_s=gd[G1]->Neighbours(),ptr_n=gi[G1]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next()) {
	 // on met le tas a jour
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=R12;
	    tmp.ex2=lut[ptr_s->Node()].no;
	    tas.Push(tmp,(float) (min_0));
	    if (tas.Full()) GarbageCollector(tas,lut);
	 }
      }
   } // fin du while
   
   // 3eme partie : reetiquetage des regions 

   // Minimisation des etiquettes.
   for (i=1;i<(int)(nb_lut+nbre);i++) {
      Ulong j=i;
      while (lut[j].no!=j) { j=lut[j].no;} 
      lut[i].no=lut[j].no_node;
   }
   // fusion dans la carte de region
   for (unsigned int f=0; f < rgd.VectorSize(); f++) {
      // pour la region qui correspond au voisin qui a fusionne, on donne le Label du 1er sommet
      p_rgd[f]=(Ulong)lut[p_rgs[f]].no;
   }
   
   gi.Delete();
   rgd.Labels(rgs.Labels());
   
   delete [] lut;
   
   return nbre; // retourne le nombre de regions fusionnees
}


/**
 *Remplissage de la lut
 */
void remplissage( const Reg3d &rgs, const Img3duc &ims, Graph3d &gs, Graph3d &gn ) {
   Point3d p;
   Ulong r1,r2;
   GEdge *ptr_s, *ptr_n;
  
   // Initialisation
   for (int i=0; i<gs.Size(); i++)
      if ((gs[i]))
	 for (ptr_s=gs[i]->Neighbours(), ptr_n=gn[i]->Neighbours(); ptr_n!=NULL; ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next())
	    ptr_s->weight= ptr_n->weight=0.0F;

   for (p.z = 1; p.z < ims.Depth() - 1; p.z++)
   for (p.y = 1; p.y < ims.Height() - 1; p.y++)
   for (p.x = 1; p.x < ims.Width() - 1; p.x++)
   {
      if (rgs[p] == 0) continue;
    
      for (int v=0; v<26; v++) {
	 r1=rgs[p];
	 r2=rgs[p+v26[v]];
	 if (r2 && (r1 != r2)) {
	    gn.Link(r1,r2,1.0F,true);
	    gs.Link(r1,r2,(float)ABS(ims[p]-ims[p+v26[v]]),true);
	 }
      }
   }
}

/**
 *  Code invariant par rapport au critere
 */
Errc PBoundaryMerging( const Reg3d &rgs, const Graph3d &gs, const Img3duc &ims, Reg3d &rgd, Graph3d &gd, Long number, float threshold ) {
 Region *lut;
   Ulong *p_rgd = rgd.Vector();
   Ulong *p_rgs = rgs.Vector();
   GEdge *ptr_s, *ptr_n;
   double min_0=0;
   Arete tmp;
   Ulong nb_lut=(rgs.Labels()+1);
   Graph3d gi;
   int i;
   Heap <Arete,float,int> tas(26*gs.Size());

   // supprimer le controle du nombre d'iterations
   if (number<0) 
      number=MAXLONG;
  
   gi=gs;
   gd=gs;

   // 1ere partie : Initialisation.
   lut = new Region [2*nb_lut-1];
   initialisation(lut,2*nb_lut-1);
   remplissage(rgs,ims,gd,gi);
   
   // on calcule la valuation des aretes, si valuation<=threshold alors on empile
   for (i=1;i<gs.Size();i++) {
      // si le noeud n'existe pas, on passe
      if (gd[i]==NULL) {
	 continue;
      }
      for (ptr_s=gd[i]->Neighbours(), ptr_n=gi[i]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(),ptr_n=ptr_n->Next()) {
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=i;
	    tmp.ex2=ptr_s->Node();
	    tas.Push(tmp,(float) (min_0));
	 }
      }
   } // fin du parcours des noeuds du graphe
   
   // 2eme partie : fusion 
   Long nbre=0;
   while (nbre<number && !tas.Empty()) {
      // On fusionne l'arc minimum du graphe precedent
      // s'il repond au critere        
      tmp=tas.Pop();
      Ulong R1=tmp.ex1;
      Ulong R2=tmp.ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if ((lut[R1].no!=R1) || (lut[R2].no!=R2)) continue;
      
      // on ne conserve que le sommet de numero minimum
      Ulong G1=MIN(lut[R1].no_node,lut[R2].no_node);
      Ulong G2=MAX(lut[R1].no_node,lut[R2].no_node);
      
      // on incremente le nombre de regions fusionnees
      // on met a jour la lut.
      Ulong R12=nb_lut+ ++nbre;
      modification(lut,R12,R1,R2);
      lut[R12].no_node=G1;
      lut[R12].no=R12;
      lut[R2].no_node=G1;
      lut[G1].no=R12;
      lut[R1].no=R12;
      lut[R2].no=R12;
      
      // on met le graphe a jour
      gd[G1]->seed=(gd[G1]->seed+gd[G2]->seed)/2;
      gd.Merge(G1,G2); // G1 reste, G2 disparait.
      gi.Merge(G1,G2); // G1 reste, G2 disparait.
      for (ptr_s=gd[G1]->Neighbours(),ptr_n=gi[G1]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next()) {
	 // on met le tas a jour
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=R12;
	    tmp.ex2=lut[ptr_s->Node()].no;
	    tas.Push(tmp,(float) (min_0));
	    if (tas.Full()) GarbageCollector(tas,lut);
	 }
      }
   } // fin du while
   
   // 3eme partie : reetiquetage des regions 

   // Minimisation des etiquettes.
   for (i=1;i<(int)(nb_lut+nbre);i++) {
      Ulong j=i;
      while (lut[j].no!=j) { j=lut[j].no;} 
      lut[i].no=lut[j].no_node;
   }
   // fusion dans la carte de region
   for (unsigned int f=0; f < rgd.VectorSize(); f++) {
      // pour la region qui correspond au voisin qui a fusionne, on donne le Label du 1er sommet
      p_rgd[f]=(Ulong)lut[p_rgs[f]].no;
   }
   
   gi.Delete();
   rgd.Labels(rgs.Labels());
   
   delete [] lut;
   
   return nbre; // retourne le nombre de regions fusionnees
}


/**
 *Remplissage de la lut
 */
void remplissage( const Reg3d &rgs, const Img3dsl &ims, Graph3d &gs, Graph3d &gn ) {
   Point3d p;
   Ulong r1,r2;
   GEdge *ptr_s, *ptr_n;
  
   // Initialisation
   for (int i=0; i<gs.Size(); i++)
      if ((gs[i]))
	 for (ptr_s=gs[i]->Neighbours(), ptr_n=gn[i]->Neighbours(); ptr_n!=NULL; ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next())
	    ptr_s->weight= ptr_n->weight=0.0F;

   for (p.z = 1; p.z < ims.Depth() - 1; p.z++)
   for (p.y = 1; p.y < ims.Height() - 1; p.y++)
   for (p.x = 1; p.x < ims.Width() - 1; p.x++)
   {
      if (rgs[p] == 0) continue;
    
      for (int v=0; v<26; v++) {
	 r1=rgs[p];
	 r2=rgs[p+v26[v]];
	 if (r2 && (r1 != r2)) {
	    gn.Link(r1,r2,1.0F,true);
	    gs.Link(r1,r2,(float)ABS(ims[p]-ims[p+v26[v]]),true);
	 }
      }
   }
}

/**
 *  Code invariant par rapport au critere
 */
Errc PBoundaryMerging( const Reg3d &rgs, const Graph3d &gs, const Img3dsl &ims, Reg3d &rgd, Graph3d &gd, Long number, float threshold ) {
 Region *lut;
   Ulong *p_rgd = rgd.Vector();
   Ulong *p_rgs = rgs.Vector();
   GEdge *ptr_s, *ptr_n;
   double min_0=0;
   Arete tmp;
   Ulong nb_lut=(rgs.Labels()+1);
   Graph3d gi;
   int i;
   Heap <Arete,float,int> tas(26*gs.Size());

   // supprimer le controle du nombre d'iterations
   if (number<0) 
      number=MAXLONG;
  
   gi=gs;
   gd=gs;

   // 1ere partie : Initialisation.
   lut = new Region [2*nb_lut-1];
   initialisation(lut,2*nb_lut-1);
   remplissage(rgs,ims,gd,gi);
   
   // on calcule la valuation des aretes, si valuation<=threshold alors on empile
   for (i=1;i<gs.Size();i++) {
      // si le noeud n'existe pas, on passe
      if (gd[i]==NULL) {
	 continue;
      }
      for (ptr_s=gd[i]->Neighbours(), ptr_n=gi[i]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(),ptr_n=ptr_n->Next()) {
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=i;
	    tmp.ex2=ptr_s->Node();
	    tas.Push(tmp,(float) (min_0));
	 }
      }
   } // fin du parcours des noeuds du graphe
   
   // 2eme partie : fusion 
   Long nbre=0;
   while (nbre<number && !tas.Empty()) {
      // On fusionne l'arc minimum du graphe precedent
      // s'il repond au critere        
      tmp=tas.Pop();
      Ulong R1=tmp.ex1;
      Ulong R2=tmp.ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if ((lut[R1].no!=R1) || (lut[R2].no!=R2)) continue;
      
      // on ne conserve que le sommet de numero minimum
      Ulong G1=MIN(lut[R1].no_node,lut[R2].no_node);
      Ulong G2=MAX(lut[R1].no_node,lut[R2].no_node);
      
      // on incremente le nombre de regions fusionnees
      // on met a jour la lut.
      Ulong R12=nb_lut+ ++nbre;
      modification(lut,R12,R1,R2);
      lut[R12].no_node=G1;
      lut[R12].no=R12;
      lut[R2].no_node=G1;
      lut[G1].no=R12;
      lut[R1].no=R12;
      lut[R2].no=R12;
      
      // on met le graphe a jour
      gd[G1]->seed=(gd[G1]->seed+gd[G2]->seed)/2;
      gd.Merge(G1,G2); // G1 reste, G2 disparait.
      gi.Merge(G1,G2); // G1 reste, G2 disparait.
      for (ptr_s=gd[G1]->Neighbours(),ptr_n=gi[G1]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next()) {
	 // on met le tas a jour
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=R12;
	    tmp.ex2=lut[ptr_s->Node()].no;
	    tas.Push(tmp,(float) (min_0));
	    if (tas.Full()) GarbageCollector(tas,lut);
	 }
      }
   } // fin du while
   
   // 3eme partie : reetiquetage des regions 

   // Minimisation des etiquettes.
   for (i=1;i<(int)(nb_lut+nbre);i++) {
      Ulong j=i;
      while (lut[j].no!=j) { j=lut[j].no;} 
      lut[i].no=lut[j].no_node;
   }
   // fusion dans la carte de region
   for (unsigned int f=0; f < rgd.VectorSize(); f++) {
      // pour la region qui correspond au voisin qui a fusionne, on donne le Label du 1er sommet
      p_rgd[f]=(Ulong)lut[p_rgs[f]].no;
   }
   
   gi.Delete();
   rgd.Labels(rgs.Labels());
   
   delete [] lut;
   
   return nbre; // retourne le nombre de regions fusionnees
}


/**
 *Remplissage de la lut
 */
void remplissage( const Reg3d &rgs, const Img3dsf &ims, Graph3d &gs, Graph3d &gn ) {
   Point3d p;
   Ulong r1,r2;
   GEdge *ptr_s, *ptr_n;
  
   // Initialisation
   for (int i=0; i<gs.Size(); i++)
      if ((gs[i]))
	 for (ptr_s=gs[i]->Neighbours(), ptr_n=gn[i]->Neighbours(); ptr_n!=NULL; ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next())
	    ptr_s->weight= ptr_n->weight=0.0F;

   for (p.z = 1; p.z < ims.Depth() - 1; p.z++)
   for (p.y = 1; p.y < ims.Height() - 1; p.y++)
   for (p.x = 1; p.x < ims.Width() - 1; p.x++)
   {
      if (rgs[p] == 0) continue;
    
      for (int v=0; v<26; v++) {
	 r1=rgs[p];
	 r2=rgs[p+v26[v]];
	 if (r2 && (r1 != r2)) {
	    gn.Link(r1,r2,1.0F,true);
	    gs.Link(r1,r2,(float)ABS(ims[p]-ims[p+v26[v]]),true);
	 }
      }
   }
}

/**
 *  Code invariant par rapport au critere
 */
Errc PBoundaryMerging( const Reg3d &rgs, const Graph3d &gs, const Img3dsf &ims, Reg3d &rgd, Graph3d &gd, Long number, float threshold ) {
 Region *lut;
   Ulong *p_rgd = rgd.Vector();
   Ulong *p_rgs = rgs.Vector();
   GEdge *ptr_s, *ptr_n;
   double min_0=0;
   Arete tmp;
   Ulong nb_lut=(rgs.Labels()+1);
   Graph3d gi;
   int i;
   Heap <Arete,float,int> tas(26*gs.Size());

   // supprimer le controle du nombre d'iterations
   if (number<0) 
      number=MAXLONG;
  
   gi=gs;
   gd=gs;

   // 1ere partie : Initialisation.
   lut = new Region [2*nb_lut-1];
   initialisation(lut,2*nb_lut-1);
   remplissage(rgs,ims,gd,gi);
   
   // on calcule la valuation des aretes, si valuation<=threshold alors on empile
   for (i=1;i<gs.Size();i++) {
      // si le noeud n'existe pas, on passe
      if (gd[i]==NULL) {
	 continue;
      }
      for (ptr_s=gd[i]->Neighbours(), ptr_n=gi[i]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(),ptr_n=ptr_n->Next()) {
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=i;
	    tmp.ex2=ptr_s->Node();
	    tas.Push(tmp,(float) (min_0));
	 }
      }
   } // fin du parcours des noeuds du graphe
   
   // 2eme partie : fusion 
   Long nbre=0;
   while (nbre<number && !tas.Empty()) {
      // On fusionne l'arc minimum du graphe precedent
      // s'il repond au critere        
      tmp=tas.Pop();
      Ulong R1=tmp.ex1;
      Ulong R2=tmp.ex2;
      
      // L'une des regions a deja ete fusionnee -> on passe.
      if ((lut[R1].no!=R1) || (lut[R2].no!=R2)) continue;
      
      // on ne conserve que le sommet de numero minimum
      Ulong G1=MIN(lut[R1].no_node,lut[R2].no_node);
      Ulong G2=MAX(lut[R1].no_node,lut[R2].no_node);
      
      // on incremente le nombre de regions fusionnees
      // on met a jour la lut.
      Ulong R12=nb_lut+ ++nbre;
      modification(lut,R12,R1,R2);
      lut[R12].no_node=G1;
      lut[R12].no=R12;
      lut[R2].no_node=G1;
      lut[G1].no=R12;
      lut[R1].no=R12;
      lut[R2].no=R12;
      
      // on met le graphe a jour
      gd[G1]->seed=(gd[G1]->seed+gd[G2]->seed)/2;
      gd.Merge(G1,G2); // G1 reste, G2 disparait.
      gi.Merge(G1,G2); // G1 reste, G2 disparait.
      for (ptr_s=gd[G1]->Neighbours(),ptr_n=gi[G1]->Neighbours();ptr_s!=NULL;ptr_s=ptr_s->Next(), ptr_n=ptr_n->Next()) {
	 // on met le tas a jour
	 min_0=valuation(ptr_s->weight,ptr_n->weight);
	 if (min_0<=threshold) {
	    tmp.ex1=R12;
	    tmp.ex2=lut[ptr_s->Node()].no;
	    tas.Push(tmp,(float) (min_0));
	    if (tas.Full()) GarbageCollector(tas,lut);
	 }
      }
   } // fin du while
   
   // 3eme partie : reetiquetage des regions 

   // Minimisation des etiquettes.
   for (i=1;i<(int)(nb_lut+nbre);i++) {
      Ulong j=i;
      while (lut[j].no!=j) { j=lut[j].no;} 
      lut[i].no=lut[j].no_node;
   }
   // fusion dans la carte de region
   for (unsigned int f=0; f < rgd.VectorSize(); f++) {
      // pour la region qui correspond au voisin qui a fusionne, on donne le Label du 1er sommet
      p_rgd[f]=(Ulong)lut[p_rgs[f]].no;
   }
   
   gi.Delete();
   rgd.Labels(rgs.Labels());
   
   delete [] lut;
   
   return nbre; // retourne le nombre de regions fusionnees
}



#ifdef MAIN

/*
 * Modify only the following constants, and the operator switches.
 */
#define	USAGE	"usage: %s number threshold [-m mask] [rg_in|-] [gr_in|-] [im_in|-] [rg_out|-] [gr_out|-]"
#define	PARC	2  // Number of parameters
#define	FINC	3  // Number of input images
#define	FOUTC	2  // Number of output images
#define	MASK	1  // Level of masking

int main( int argc, char *argv[] ) {
   Errc result;                // The result code of the execution.
   Pobject* mask;              // The region map.
   Pobject* objin[FINC + 1];   // The input objects.
   Pobject* objs[FINC + 1];    // The source objects masked.
   Pobject* objout[FOUTC + 1]; // The output object.
   Pobject* objd[FOUTC + 1];   // The result object of the execution.
   char* parv[PARC + 1];       // The input parameters.

   ReadArgs(argc, argv, PARC, FINC, FOUTC, &mask, objin, objs, objout, objd, parv, USAGE, MASK); 
   // verifier toutes les entrees
   if (objs[0]->Type()==Po_Reg2d && objs[1]->Type()==Po_Graph2d && objs[2]->Type() == Po_Img2duc) {
      Reg2d* const rgs=(Reg2d*)objs[0];  
      Graph2d* const gs=(Graph2d*)objs[1];  
      Img2duc* const ims=(Img2duc*)objs[2];
      objd[0]=new Reg2d(rgs->Size());
      Reg2d* const rgd=(Reg2d*)objd[0];
      objd[1]=new Graph2d(gs->Size());
      Graph2d* const gd=(Graph2d*)objd[1];
      
      result = PBoundaryMerging(*rgs,*gs,*ims,*rgd,*gd,(Long)atoi(parv[0]),(float)atof(parv[1]));
      goto end;
   }
   // verifier toutes les entrees
   if (objs[0]->Type()==Po_Reg2d && objs[1]->Type()==Po_Graph2d && objs[2]->Type() == Po_Img2dsl) {
      Reg2d* const rgs=(Reg2d*)objs[0];  
      Graph2d* const gs=(Graph2d*)objs[1];  
      Img2dsl* const ims=(Img2dsl*)objs[2];
      objd[0]=new Reg2d(rgs->Size());
      Reg2d* const rgd=(Reg2d*)objd[0];
      objd[1]=new Graph2d(gs->Size());
      Graph2d* const gd=(Graph2d*)objd[1];
      
      result = PBoundaryMerging(*rgs,*gs,*ims,*rgd,*gd,(Long)atoi(parv[0]),(float)atof(parv[1]));
      goto end;
   }
   // verifier toutes les entrees
   if (objs[0]->Type()==Po_Reg2d && objs[1]->Type()==Po_Graph2d && objs[2]->Type() == Po_Img2dsf) {
      Reg2d* const rgs=(Reg2d*)objs[0];  
      Graph2d* const gs=(Graph2d*)objs[1];  
      Img2dsf* const ims=(Img2dsf*)objs[2];
      objd[0]=new Reg2d(rgs->Size());
      Reg2d* const rgd=(Reg2d*)objd[0];
      objd[1]=new Graph2d(gs->Size());
      Graph2d* const gd=(Graph2d*)objd[1];
      
      result = PBoundaryMerging(*rgs,*gs,*ims,*rgd,*gd,(Long)atoi(parv[0]),(float)atof(parv[1]));
      goto end;
   }
   // verifier toutes les entrees
   if (objs[0]->Type()==Po_Reg3d && objs[1]->Type()==Po_Graph3d && objs[2]->Type() == Po_Img3duc) {
      Reg3d* const rgs=(Reg3d*)objs[0];  
      Graph3d* const gs=(Graph3d*)objs[1];  
      Img3duc* const ims=(Img3duc*)objs[2];
      objd[0]=new Reg3d(rgs->Size());
      Reg3d* const rgd=(Reg3d*)objd[0];
      objd[1]=new Graph3d(gs->Size());
      Graph3d* const gd=(Graph3d*)objd[1];
      
      result = PBoundaryMerging(*rgs,*gs,*ims,*rgd,*gd,(Long)atoi(parv[0]),(float)atof(parv[1]));
      goto end;
   }
   // verifier toutes les entrees
   if (objs[0]->Type()==Po_Reg3d && objs[1]->Type()==Po_Graph3d && objs[2]->Type() == Po_Img3dsl) {
      Reg3d* const rgs=(Reg3d*)objs[0];  
      Graph3d* const gs=(Graph3d*)objs[1];  
      Img3dsl* const ims=(Img3dsl*)objs[2];
      objd[0]=new Reg3d(rgs->Size());
      Reg3d* const rgd=(Reg3d*)objd[0];
      objd[1]=new Graph3d(gs->Size());
      Graph3d* const gd=(Graph3d*)objd[1];
      
      result = PBoundaryMerging(*rgs,*gs,*ims,*rgd,*gd,(Long)atoi(parv[0]),(float)atof(parv[1]));
      goto end;
   }
   // verifier toutes les entrees
   if (objs[0]->Type()==Po_Reg3d && objs[1]->Type()==Po_Graph3d && objs[2]->Type() == Po_Img3dsf) {
      Reg3d* const rgs=(Reg3d*)objs[0];  
      Graph3d* const gs=(Graph3d*)objs[1];  
      Img3dsf* const ims=(Img3dsf*)objs[2];
      objd[0]=new Reg3d(rgs->Size());
      Reg3d* const rgd=(Reg3d*)objd[0];
      objd[1]=new Graph3d(gs->Size());
      Graph3d* const gd=(Graph3d*)objd[1];
      
      result = PBoundaryMerging(*rgs,*gs,*ims,*rgd,*gd,(Long)atoi(parv[0]),(float)atof(parv[1]));
      goto end;
   }
  {
     PrintErrorFormat(objin, FINC, argv); 
     result = FAILURE; 
  }	

end:
  if (result) {
	WriteArgs(argc, argv, PARC, FINC, FOUTC, &mask, objin, objs, objout, objd, MASK); 
  }
  Exit(result); 
  return 0; 
}
#endif
