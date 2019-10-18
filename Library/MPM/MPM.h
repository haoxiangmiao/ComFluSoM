/************************************************************************
 * ComFluSoM - Simulation kit for Fluid Solid Soil Mechanics            *
 * Copyright (C) 2019 Pei Zhang                                         *
 * Email: peizhang.hhu@gmail.com                                        *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * any later version.                                                   *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>  *
 ************************************************************************/

#include "../HEADER.h"
#include <SHAPE.h>
#include <MPM_PARTICLE.h>
#include <MPM_NODE.h>

class MPM
{
public:
	MPM();
	~MPM();
	MPM(size_t ntype, size_t cmtype, size_t nx, size_t ny, size_t nz, Vector3d dx);
	void Init();
	void UpdateLn(MPM_PARTICLE* p0);
	void CalNGN(MPM_PARTICLE* p0);
	void ParticleToNode();
	void CalFOnNode(bool firstStep);
	void NodeToParticle();
	void CalStressOnParticleElastic();
	void CalStressOnParticleMohrCoulomb();
	void CalStressOnParticleNewtonian();
	void CalVGradLocal(int p);
	void CalPSizeCP(int p);
	void CalPSizeR(int p);
	void CalVOnNode();
	void CalVOnNodeDoubleMapping();
	void SetNonSlippingBC(size_t n);
	void SetNonSlippingBC(size_t i, size_t j, size_t k);
	void SetSlippingBC(size_t n, Vector3d& norm);
	void SetSlippingBC(size_t i, size_t j, size_t k, Vector3d& norm);
	void SetFrictionBC(size_t n, double mu, Vector3d& norm);
	void SetFrictionBC(size_t i, size_t j, size_t k, double mu, Vector3d& norm);
	void SolveMUSL(int tt, int ts);
	void SolveUSF(int tt, int ts);
	void SolveUSA(int tt, int ts);
	void AddNode(size_t level, Vector3d& x);
	void AddParticle(int tag, Vector3d& x, double m, double young, double poisson);
	void DeleteParticles();
	void AddBoxParticles(Vector3d& x0, Vector3d& l, double ratio, double m, double young, double poisson);
	void WriteFileH5(int n);
	void FindIndex(size_t n, size_t& i, size_t& j, size_t& k);

	double 		(*N)(Vector3d& x, Vector3d& xc, Vector3d& l, Vector3d& lp);
	Vector3d 	(*GN)(Vector3d& x, Vector3d& xc, Vector3d& l, Vector3d& lp);

	vector <size_t>					LAn;													// List of actived nodes
	vector <MPM_PARTICLE*>			Lp;														// List of all MPM particles
	vector <MPM_PARTICLE*>			Lbp;													// List of boundary MPM particles
	vector <MPM_NODE*>				Ln;														// List of all MPM nodes

	bool							Periodic[3];

    size_t 							Nx;														// Domain size
    size_t 							Ny;
    size_t 							Nz;
    size_t 							Ncz;
    size_t 							Ncy;
    size_t 							Nnode;													// Total number of nodes

    size_t 							Nproc;
    size_t 							D;														// Dimension	
    size_t 							Ntype;													// Type of shape function 0 for Linear, 1 for Quadratic and 2 for Cubic 3 for GIMP
    size_t							CMType;													// Constitutive Model Type, 0 for elastic, 1 for Mohr-Coulomb

    double 							Nrange;													// Influence range of shape function
    double 							Dt;														// Time step
    double 							Dc;														// Damping coefficient
    double 							C;														// Speed of sound
    Vector3d						Dx;														// Space step
};

MPM::MPM(size_t ntype, size_t cmtype, size_t nx, size_t ny, size_t nz, Vector3d dx)
{
	Nproc	= 1;

	Ntype 	= ntype;
	CMType 	= cmtype;
	if (CMType==0)			cout << "Using Elastic model." << endl;
	else if (CMType==1)		cout << "Using MohrCoulomb model." << endl;
	else if (CMType==2)		cout << "Using Newtonian model." << endl;
	else
	{
		cout << "Undefined Constitutive model."<< endl;
		abort();
	}
	Nx 		= nx;
	Ny 		= ny;
	Nz 		= nz;
	Dx 		= dx;
	D 		= 3;
	Dt 		= 1.;
	Dc 		= 0.;
	C 		= 0.;
	Periodic[0] = false;
	Periodic[1] = false;
	Periodic[2] = false;

	Ncz = (Nx+1)*(Ny+1);
	Ncy = (Nx+1);

	Nnode = (Nx+1)*(Ny+1)*(Nz+1);

	if (Nz==0)
	{
		D = 2;
		if (Ny==0)	D = 1;
	}
	// Linear
	if 		(Ntype == 0)
	{
		if (D==1)
		{
			N  		=& ShapeL1D;
			GN 		=& GradShapeL1D;			
		}
		else if (D==2)
		{
			N  		=& ShapeL2D;
			GN 		=& GradShapeL2D;
		}
		else if (D==3)
		{
			N  		=& ShapeL3D;
			GN 		=& GradShapeL3D;
		}
		else
		{
			cout << "Dimension is higher than 3." << endl;
			abort();
		}
		Nrange 	= 1.;
		cout << "Using Linear shape function." << endl;
	}
	// Quadratic
	else if (Ntype == 1)
	{
		if (D==1)
		{
			N  		=& ShapeQ1D;
			GN 		=& GradShapeQ1D;			
		}
		else if (D==2)
		{
			N  		=& ShapeQ2D;
			GN 		=& GradShapeL2D;
		}
		else if (D==3)
		{
			N  		=& ShapeQ3D;
			GN 		=& GradShapeQ3D;
		}
		else
		{
			cout << "Dimension is higher than 3." << endl;
			abort();
		}
		Nrange 	= 1.5;
		cout << "Using Quadratic shape function." << endl;
	}
	// Cubic
	else if (Ntype == 2)
	{
		if (D==1)
		{
			N  		=& ShapeC1D;
			GN 		=& GradShapeC1D;			
		}
		else if (D==2)
		{
			N  		=& ShapeC2D;
			GN 		=& GradShapeC2D;
		}
		else if (D==3)
		{
			N  		=& ShapeC3D;
			GN 		=& GradShapeC3D;
		}
		else
		{
			cout << "Dimension is higher than 3." << endl;
			abort();
		}
		Nrange 	= 2.;
		cout << "Using Cubic shape function." << endl;
	}
	// GIMP
	else if (Ntype == 3)
	{
		if (D==1)
		{
			N  		=& ShapeGIMP1D;
			GN 		=& GradShapeGIMP1D;			
		}
		else if (D==2)
		{
			N  		=& ShapeGIMP2D;
			GN 		=& GradShapeGIMP2D;
		}
		else if (D==3)
		{
			N  		=& ShapeGIMP3D;
			GN 		=& GradShapeGIMP3D;
		}
		else
		{
			cout << "Dimension is higher than 3." << endl;
			abort();
		}
		Nrange 	= 1.;
		cout << "Using GIMP shape function." << endl;
	}
	else
	{
		cout << "Undefined shape function type. Retry 0 for Linear, 1 for Quadratic and 2 for Cubic." << endl;
		abort();
	}
}

void MPM::Init()
{
	cout << "================ Start init.  ================" << endl;
	Lp.resize(0);
	Ln.resize(0);
	// Add basic nodes
	for (size_t n=0; n<(Nx+1)*(Ny+1)*(Nz+1); ++n)
	{
    	size_t i, j, k;
    	FindIndex(n, i, j, k);
		Vector3d x (i, j, k);
		AddNode(0, x);
	}
	cout << "=============== Finish init.  ================" << endl;
}
// Find index for grid
inline void MPM::FindIndex(size_t n, size_t& i, size_t& j, size_t& k)
{
	k = n/Ncz;
	j = (n%Ncz)/Ncy;
	i = (n%Ncz)%Ncy;
}

// void MPM::UpdateLn(MPM_PARTICLE* p0)
// {
// 	p0->Lni.resize(0);
// 	// Find min position of nodes which is infuenced by this particle
// 	Vector3i minx 	= Vector3i::Zero();
// 	Vector3i maxx 	= Vector3i::Zero();
// 	for (size_t d=0; d<D; ++d)
// 	{
// 		minx(d) = (int) trunc(p0->X(d) - p0->PSize(d)-1.);
// 		maxx(d) = (int) ceil(p0->X(d) + p0->PSize(d)+1.);
// 	}
// 	// Find nodes within the influence range
// 	for (int i=minx(0); i<=maxx(0); ++i)
// 	for (int j=minx(1); j<=maxx(1); ++j)
// 	for (int k=minx(2); k<=maxx(2); ++k)
// 	{
// 		// Find id of current node
// 		int id = i+j*Ncy+k*Ncz;
// 		p0->Lni.push_back(id);
// 	}
// }

void MPM::CalNGN(MPM_PARTICLE* p0)
{
	// Reset shape function (N) and gradient of shape function (GN)
	p0->Lni.resize(0);
	p0->LnN.resize(0);
	p0->LnGN.resize(0);
	// Find min position of nodes which is infuenced by this particle
	Vector3i minx 	= Vector3i::Zero();
	Vector3i maxx 	= Vector3i::Zero();
	for (size_t d=0; d<D; ++d)
	{
		minx(d) = (int) trunc(p0->X(d) - p0->PSize(d)-1.);
		maxx(d) = (int) ceil(p0->X(d) + p0->PSize(d)+1.);
	}

	// Find nodes within the influence range
	for (int i=minx(0); i<=maxx(0); ++i)
	for (int j=minx(1); j<=maxx(1); ++j)
	for (int k=minx(2); k<=maxx(2); ++k)
	{
		double n;
		Vector3d gn;
		Vector3d xn (i,j,k);
		GIMP3D(p0->X, xn, Dx, p0->PSize, n, gn);
		if (n>0.)
		{
			// Find id of current node
			int id = i+j*Ncy+k*Ncz;
			p0->Lni.push_back(id);
			p0->LnN.push_back(n);
			p0->LnGN.push_back(gn);
		}
	}

	// for (size_t l=0; l<p0->Lni.size(); ++l)
	// {
	// 	// Calculate shape function (N) and gradient of shape function (GN)
	// 	Vector3d xc = Ln[p0->Lni[l]]->X;
	// 	double n;
	// 	Vector3d gn;
	// 	GIMP3D(p0->X, xc, Dx, p0->PSize, n, gn);
	// 	p0->LnN.push_back(n);
	// 	p0->LnGN.push_back(gn);
	// }
}

void MPM::ParticleToNode()
{
	// reset mass internal force velocity for nodes
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t n=0; n<LAn.size(); ++n)
	{
		size_t id = LAn[n];
		Ln[id]->Reset();
    }
    LAn.resize(0);
    // Update shape function and grad
    #pragma omp parallel for schedule(static) num_threads(Nproc)
    for (size_t p=0; p<Lp.size(); ++p)
    {
    	// UpdateLn(Lp[p]);
    	CalNGN(Lp[p]);

    	Matrix3d vsp = -Lp[p]->Vol*Lp[p]->S;
		Vector3d fex = Lp[p]->M*Lp[p]->B + Lp[p]->Fh;

		for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
		{
			// Grid id
			size_t id = Lp[p]->Lni[l];
			// weight
			double 		n 	= Lp[p]->LnN[l];
			Vector3d 	gn 	= Lp[p]->LnGN[l];
			Vector3d 	df 	= n*fex + vsp*gn;
			// weigthed mass contribution
			double nm = n*Lp[p]->M;

			if (nm<0.)
			{
				cout << "mass problem" << endl;
				cout << "nm= " << nm << endl;
				cout << "p= " << p << endl;
				cout << "id= " << id << endl;
				cout << "Lp[p]->X= " << Lp[p]->X.transpose() << endl;
				cout << "Ln[id]->X= " << Ln[id]->X.transpose() << endl;
				abort();
			}
			// #pragma omp critical
			// {
			// 	Ln[id]->M += nm;
			// 	Ln[id]->Mv += nm*Lp[p]->V;
			// 	Ln[id]->F += df;
			// 	Ln[id]->Mv += df*Dt;
			// }
			#pragma omp atomic
			Ln[id]->M += nm;
			#pragma omp atomic
			Ln[id]->Mv(0) += nm*Lp[p]->V(0);
			#pragma omp atomic
			Ln[id]->Mv(1) += nm*Lp[p]->V(1);
			#pragma omp atomic
			Ln[id]->Mv(2) += nm*Lp[p]->V(2);
			#pragma omp atomic
			Ln[id]->F(0) += df(0);
			#pragma omp atomic
			Ln[id]->F(1) += df(1);
			#pragma omp atomic
			Ln[id]->F(2) += df(2);
			#pragma omp atomic
			Ln[id]->Mv(0) += df(0)*Dt;
			#pragma omp atomic
			Ln[id]->Mv(1) += df(1)*Dt;
			#pragma omp atomic
			Ln[id]->Mv(2) += df(2)*Dt;
		}
    }

	vector<vector <size_t>> lan;
	lan.resize(Nproc);
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t p=0; p<Lp.size(); ++p)
	{
		auto id = omp_get_thread_num();
		lan[id].insert( lan[id].end(), Lp[p]->Lni.begin(), Lp[p]->Lni.end() );
	}
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t n=0; n<Nproc; ++n)
	{
		sort( lan[n].begin(), lan[n].end() );
		lan[n].erase(unique(lan[n].begin(), lan[n].end()), lan[n].end());
	}
	for (size_t n=0; n<Nproc; ++n)
	{
		LAn.insert( LAn.end(), lan[n].begin(), lan[n].end() );
	}
	sort( LAn.begin(), LAn.end() );
	LAn.erase(unique(LAn.begin(), LAn.end()), LAn.end());
}

void MPM::CalFOnNode(bool firstStep)
{
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t p=0; p<Lp.size(); ++p)
	{
		Matrix3d vsp = -Lp[p]->Vol*Lp[p]->S;
		Vector3d fex = Lp[p]->M*Lp[p]->B + Lp[p]->Fh;
		for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
		{
			size_t 		id 	= Lp[p]->Lni[l];
			double 		n   = Lp[p]->LnN[l];
			Vector3d 	gn 	= Lp[p]->LnGN[l];
			Vector3d 	df  = n*fex + vsp*gn;
			if (firstStep)	df *= 0.5;

			#pragma omp atomic
			Ln[id]->F(0) += df(0);
			#pragma omp atomic
			Ln[id]->F(1) += df(1);
			#pragma omp atomic
			Ln[id]->F(2) += df(2);
			#pragma omp atomic
			Ln[id]->Mv(0) += df(0)*Dt;
			#pragma omp atomic
			Ln[id]->Mv(1) += df(1)*Dt;
			#pragma omp atomic
			Ln[id]->Mv(2) += df(2)*Dt;
		}	
	}
}

void MPM::CalVOnNode()
{
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t n=0; n<LAn.size(); ++n)
	{
		size_t id = LAn[n];
		if (Ln[id]->M==0.)	Ln[id]->V.setZero();
		else
		{
			Vector3d fdamp = Dc*Ln[id]->F.norm()*Ln[id]->Mv.normalized();
			// cout << "fdamp= " << fdamp.transpose() << endl;
			Ln[id]->F  -= fdamp;
			Ln[id]->Mv -= fdamp*Dt;
			// Apply slipping BC
			if (Ln[id]->BCTypes.size()>0)
			{
				for (size_t i=0; i<Ln[id]->BCTypes.size(); ++i)
				{
					if (Ln[id]->BCTypes[i]==1)			Ln[id]->NonSlippingBC();
					else if (Ln[id]->BCTypes[i]==2)		Ln[id]->SlippingBC(Ln[id]->Norms[i]);
					else if (Ln[id]->BCTypes[i]==3)		Ln[id]->FrictionBC(Dt, Ln[id]->Norms[i]);
				}
			}
			Ln[id]->V = Ln[id]->Mv/Ln[id]->M;
			if (Ln[id]->M<1.0e-12)
			{
				// cout << "very small node mass" << endl;
				Ln[id]->V.setZero();
			}
		}
	}
}
void MPM::SetNonSlippingBC(size_t n)
{
	Ln[n]->BCTypes.push_back(1);
}
void MPM::SetNonSlippingBC(size_t i, size_t j, size_t k)
{
	int n = i+j*Ncy+k*Ncz;
	Ln[n]->BCTypes.push_back(1);
}

void MPM::SetSlippingBC(size_t n, Vector3d& norm)
{
	Ln[n]->BCTypes.push_back(2);
	Ln[n]->Norms.push_back(norm);
}
void MPM::SetSlippingBC(size_t i, size_t j, size_t k, Vector3d& norm)
{
	int n = i+j*Ncy+k*Ncz;
	Ln[n]->BCTypes.push_back(2);
	Ln[n]->Norms.push_back(norm);
}

void MPM::SetFrictionBC(size_t n, double mu, Vector3d& norm)
{
	Ln[n]->BCTypes.push_back(3);
	Ln[n]->Norms.push_back(norm);
	Ln[n]->Mu = mu;

}
void MPM::SetFrictionBC(size_t i, size_t j, size_t k, double mu, Vector3d& norm)
{
	int n = i+j*Ncy+k*Ncz;
	Ln[n]->BCTypes.push_back(3);
	Ln[n]->Norms.push_back(norm);
	Ln[n]->Mu = mu;
}

void MPM::NodeToParticle()
{
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t p=0; p<Lp.size(); ++p)
	{
		// Reset position increasement of this particle
		Lp[p]->DeltaX 	= Vector3d::Zero();
		if (!Lp[p]->FixV)
		{
			for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
			{
				size_t 	id = Lp[p]->Lni[l];
				double 	n  = Lp[p]->LnN[l];
				// Update velocity of this particle
				// Vector3d fdamp = Dc*Ln[id]->F.norm()*Ln[n]->Mv.normalized();
				// Vector3d ai = (Ln[id]->F-fdamp)/Ln[id]->M;
				if (n>0.)
				{
					Vector3d an = Ln[id]->F/Ln[id]->M;
					Lp[p]->V += n*an*Dt;
					Lp[p]->X += n*Ln[id]->V*Dt;
				}
				// Vector3d ai = Ln[id]->F/Ln[id]->M;
				// Lp[p]->V += n*ai*Dt;
				// Lp[p]->X += n*Ln[id]->V*Dt;
			}
		}
		else
		{
			Lp[p]->V = Lp[p]->Vf;
			Lp[p]->X += Lp[p]->V*Dt;
		}
	}		
}

// void MPM::CalVOnNodeDoubleMapping()
// {
// 	#pragma omp parallel for schedule(static) num_threads(Nproc)
// 	for (size_t c=0; c<LAn.size(); ++c)
// 	{
// 		int i = LAn[c][0];
// 		int j = LAn[c][1];
// 		int k = LAn[c][2];
// 		V[i][j][k] = Vector3d::Zero();
// 	}
// 	// Double map velocity from particles to nodes to aviod small mass problem
// 	#pragma omp parallel for schedule(static) num_threads(Nproc)
// 	for (size_t p=0; p<Lp.size(); ++p)
// 	{
// 		for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
// 		{
// 			Vector3i ind = Lp[p]->Lni[l];
// 			double n = Lp[p]->LnN[l];
// 			// Update velocity of this node
// 			#pragma omp critical
// 			{
// 				V[ind(0)][ind(1)][ind(2)] += n*Lp[p]->M*Lp[p]->V/M[ind(0)][ind(1)][ind(2)];		
// 			}
// 		}
// 	}
// }

void MPM::CalVGradLocal(int p)
{
	Lp[p]->L = Matrix3d::Zero();
	for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
	{
		size_t	 	id = Lp[p]->Lni[l];
		Vector3d 	gn 	= Lp[p]->LnGN[l];
		// Calculate velocity gradient tensor
		Lp[p]->L += gn*Ln[id]->V.transpose();
	}
}

void MPM::CalPSizeCP(int p)
{
	Lp[p]->PSize(0) =  Lp[p]->PSize0(0)*Lp[p]->F(0,0);
	Lp[p]->PSize(1) =  Lp[p]->PSize0(1)*Lp[p]->F(1,1);
	Lp[p]->PSize(2) =  Lp[p]->PSize0(2)*Lp[p]->F(2,2);
}

// Based on "iGIMP: An implicit generalised interpolation material point method for large deformations"
void MPM::CalPSizeR(int p)
{
	Lp[p]->PSize(0) =  Lp[p]->PSize0(0)*sqrt(Lp[p]->F(0,0)*Lp[p]->F(0,0) + Lp[p]->F(1,0)*Lp[p]->F(1,0) + Lp[p]->F(2,0)*Lp[p]->F(2,0));
	Lp[p]->PSize(1) =  Lp[p]->PSize0(1)*sqrt(Lp[p]->F(0,1)*Lp[p]->F(0,1) + Lp[p]->F(1,1)*Lp[p]->F(1,1) + Lp[p]->F(2,1)*Lp[p]->F(2,1));
	Lp[p]->PSize(2) =  Lp[p]->PSize0(2)*sqrt(Lp[p]->F(0,2)*Lp[p]->F(0,2) + Lp[p]->F(1,2)*Lp[p]->F(1,2) + Lp[p]->F(2,2)*Lp[p]->F(2,2));
}

void MPM::CalStressOnParticleElastic()
{
	// Update stresses on particles
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t p=0; p<Lp.size(); ++p)
	{
		// Velocity gradient tensor
		CalVGradLocal(p);
		// Update deformation tensor
		Lp[p]->F = (Matrix3d::Identity() + Lp[p]->L*Dt)*Lp[p]->F;
		// Update particle length
		CalPSizeR(p);
		// CalPSizeCP(p);
		// Update volume of particles
		Lp[p]->Vol 	= Lp[p]->F.determinant()*Lp[p]->Vol0;
		// Update strain
		Matrix3d de = 0.5*Dt*(Lp[p]->L + Lp[p]->L.transpose());
		// Update stress
		Matrix3d w = 0.5*Dt*((Lp[p]->L - Lp[p]->L.transpose()));
		Lp[p]->S += w*Lp[p]->S-Lp[p]->S*w.transpose();
		Lp[p]->Elastic(de);
	}
}

void MPM::CalStressOnParticleMohrCoulomb()
{
	// Update stresses on particles
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t p=0; p<Lp.size(); ++p)
	{
		// Velocity gradient tensor
		CalVGradLocal(p);
		// Update deformation tensor
		Lp[p]->F = (Matrix3d::Identity() + Lp[p]->L)*Lp[p]->F;
		// Update particle length
		CalPSizeR(p);
		// CalPSizeCP(p);
		// Update volume of particles
		Lp[p]->Vol 	= Lp[p]->F.determinant()*Lp[p]->Vol0;
		// Update strain
		Matrix3d de = 0.5*Dt*(Lp[p]->L + Lp[p]->L.transpose());
		// Update stress
		Lp[p]->MohrCoulomb(de);
	}
}

void MPM::CalStressOnParticleNewtonian()
{
	// cout << "start CalStressOnParticleNewtonian " << endl;
	// Update stresses on particles
	#pragma omp parallel for schedule(static) num_threads(Nproc)
	for (size_t p=0; p<Lp.size(); ++p)
	{
		// Velocity gradient tensor
		CalVGradLocal(p);
		// Update deformation tensor
		Lp[p]->F = (Matrix3d::Identity() + Lp[p]->L*Dt)*Lp[p]->F;
		// Update particle length
		CalPSizeR(p);
		// Update volume of particles
		Lp[p]->Vol 	= Lp[p]->F.determinant()*Lp[p]->Vol0;
		// Update strain
		Matrix3d de = 0.5*Dt*(Lp[p]->L + Lp[p]->L.transpose());
		// Update EOS
		// cout << "start EOSMorris " << endl;
		// Lp[p]->EOSMorris(C);
		Lp[p]->EOSMonaghan(C);
		// Update stress
		// cout << "start Newtonian " << endl;
		Lp[p]->Newtonian(de);
	// cout << "finish CalStressOnParticleNewtonian " << endl;

	}
}

void MPM::SolveMUSL(int tt, int ts)
{
	for (int t=0; t<tt; ++t)
	{
		bool show = false;
		if (t%100==0)	show = true;
		if (show) 	cout << "Time Step = " << t << endl;
		// if (t> 11200)
		if (t%ts == 0)
		{
			cout << "*****************Time Step = " << t << "*******************" << endl;
			WriteFileH5(t);
		}
		// cout << "1" << endl;
		auto t_start = std::chrono::system_clock::now();
		ParticleToNode();
		auto t_end = std::chrono::system_clock::now();
		if (show)	cout << "ParticleToNode= " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << endl;
		// cout << "2" << endl;
		// t_start = std::chrono::system_clock::now();
		// if (t==0)	CalFOnNode(true);
		// else 		CalFOnNode(false);
		// t_end = std::chrono::system_clock::now();
		// if (show)	cout << "CalFOnNode= " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << endl;
		// cout << "3" << endl;
		// cout << "4" << endl;
		t_start = std::chrono::system_clock::now();
		CalVOnNode();
		t_end = std::chrono::system_clock::now();
		if (show)	cout << "CalVOnNode= " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << endl;
		t_start = std::chrono::system_clock::now();
		NodeToParticle();

		t_end = std::chrono::system_clock::now();
		if (show)	cout << "NodeToParticle= " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << endl;
		// cout << "5" << endl;
		// CalVOnNode();
		// cout << "6" << endl;
		t_start = std::chrono::system_clock::now();
		if 		(CMType==0) 	CalStressOnParticleElastic();
		else if (CMType==1) 	CalStressOnParticleMohrCoulomb();
		else if (CMType==2) 	CalStressOnParticleNewtonian();
		// if (t>11490)
		// {
		// 	int p=60590;
		// 	for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
		// 	{
		// 		size_t	 	id = Lp[p]->Lni[l];
		// 		Vector3d 	gn 	= Lp[p]->LnGN[l];
		// 		cout << "Xn= "<< Ln[id]->X.transpose() << endl;
		// 		cout << "gn= " << gn << endl;
		// 		cout << "Vn= "<< Ln[id]->V.transpose() << endl;
		// 		cout << "============" << endl;
		// 	}
		// 	cout << "Lp[p]->L: " << endl;
		// 	cout << Lp[p]->L << endl;
		// 	cout << "============" << endl;
		// 	cout << "Lp[p]->S: " << endl;
		// 	cout << Lp[p]->S << endl;
		// 	cout << "============" << endl;
		// 	cout << "de= " << endl;
		// 	cout << 0.5*Dt*(Lp[p]->L + Lp[p]->L.transpose()) << endl;
		// 	cout << "============" << endl;
		// }
		t_end = std::chrono::system_clock::now();
		// auto t_end = std::chrono::system_clock::now();
		if (show)	cout << "CalStressOnParticleMohrCoulomb= " << std::chrono::duration<double, std::milli>(t_end-t_start).count() << endl;
		if (show) 	cout << "===========================" << endl;

		// if (t> 11300)
		// {
		// 	for (size_t p=0; p<Lp.size(); ++p)
		// 	{
		// 		if (Lp[p]->X(2)>26.5)
		// 		{
		// 			cout << "==========================" << endl;
		// 			cout << "p= " << p << endl;
		// 			cout << "Lp[p]->X= " << Lp[p]->X.transpose() << endl;
		// 			cout << "Lp[p]->V= " << Lp[p]->V.transpose() << endl;
		// 			cout << "Lp[p]->L= " << Lp[p]->L << endl;

		// 			bool stop = false;
		// 			for (size_t l=0; l<Lp[p]->Lni.size(); ++l)
		// 			{
		// 				size_t 		id 	= Lp[p]->Lni[l];
		// 				cout << "Ln[l]->M= " << Ln[id]->M << endl;
		// 				cout << "Ln[l]->X= " << Ln[id]->X.transpose() << endl;
		// 				cout << "Ln[l]->V= " << Ln[id]->V.transpose() << endl;
		// 				cout << "Ln[l]->Mv= " << Ln[id]->Mv.transpose() << endl;
		// 				Vector3d dis = Lp[p]->X-Ln[id]->X;
		// 				if (abs(dis(0))-1.>Lp[p]->PSize(0) || abs(dis(1))-1.>Lp[p]->PSize(1) || abs(dis(2))-1.>Lp[p]->PSize(2))
		// 				{
		// 					cout << "this node out of range" << endl;
		// 					stop = true;
		// 					break;
		// 				}
		// 			}
		// 			if (stop)
		// 			{
		// 				abort();
		// 				// cout << "Lp[p]->Lni.size()= " << Lp[p]->Lni.size() << endl;
		// 				// Vector3i minx 	= Vector3i::Zero();
		// 				// Vector3i maxx 	= Vector3i::Zero();
		// 				// for (size_t d=0; d<D; ++d)
		// 				// {
		// 				// 	minx(d) = (int) trunc(Lp[p]->X(d) - Lp[p]->PSize(d)-1.);
		// 				// 	maxx(d) = (int) ceil(Lp[p]->X(d) + Lp[p]->PSize(d)+1.);
		// 				// }
		// 			}
		// 		}
		// 		if (Lp[p]->V.norm()>0.03)
		// 		{
		// 			cout << "velocity larger than 0.03" << endl;
		// 			abort();
		// 		}
		// 	}
		// }
	}
}

// void MPM::SolveUSF(int tt, int ts)
// {
// 	for (int t=0; t<tt; ++t)
// 	{
// 		if (t%ts == 0)
// 		{
// 			cout << "Time Step = " << t << endl;
// 			WriteFileH5(t);
// 		}

// 		ParticleToNode();
// 		CalVOnNode();

// 		for (size_t i=0; i<LFn.size(); ++i)
// 		{
// 			V[LFn[i][0]][LFn[i][1]][LFn[i][2]](LFn[i][3]) = 0.;
// 		}

// 		if 		(CMType==0) 	CalStressOnParticleElastic();
// 		else if (CMType==1) 	CalStressOnParticleMohrCoulomb();
// 		CalFOnNode(false);

// 		for (size_t i=0; i<LFn.size(); ++i)
// 		{
// 			F[LFn[i][0]][LFn[i][1]][LFn[i][2]](LFn[i][3]) = 0.;
// 			Mv[LFn[i][0]][LFn[i][1]][LFn[i][2]](LFn[i][3]) = 0.;
// 		}

// 		NodeToParticle();
// 	}
// }

// void MPM::SolveUSA(int tt, int ts)
// {
// 	for (int t=0; t<tt; ++t)
// 	{
// 		if (t%ts == 0)
// 		{
// 			cout << "Time Step = " << t << endl;
// 			WriteFileH5(t);
// 		}

// 		ParticleToNode();
// 		CalVOnNode();

// 		for (size_t i=0; i<LFn.size(); ++i)
// 		{
// 			V[LFn[i][0]][LFn[i][1]][LFn[i][2]](LFn[i][3]) = 0.;
// 		}

// 		#pragma omp parallel for schedule(static) num_threads(Nproc)
// 		for (size_t p=0; p<Lp.size(); ++p)
// 		{
// 			CalVGradLocal(p);
// 			// Update strain
// 			Matrix3d de = 0.25*(Lp[p]->L + Lp[p]->L.transpose());
// 			// Update stress
// 			Lp[p]->S += 2.*Lp[p]->Mu*de + Lp[p]->La*de.trace()*Matrix3d::Identity();
// 		}

// 		CalFOnNode(false);

// 		for (size_t i=0; i<LFn.size(); ++i)
// 		{
// 			F[LFn[i][0]][LFn[i][1]][LFn[i][2]](LFn[i][3]) = 0.;
// 			Mv[LFn[i][0]][LFn[i][1]][LFn[i][2]](LFn[i][3]) = 0.;
// 		}

// 		NodeToParticle();
// 		CalVOnNodeDoubleMapping();

// 		#pragma omp parallel for schedule(static) num_threads(Nproc)
// 		for (size_t p=0; p<Lp.size(); ++p)
// 		{	
// 			Matrix3d L0 = Lp[p]->L;
// 			CalVGradLocal(p);
// 			// Update strain
// 			Matrix3d de = 0.25*(Lp[p]->L + Lp[p]->L.transpose());
// 			// Update stress
// 			Lp[p]->S += 2.*Lp[p]->Mu*de + Lp[p]->La*de.trace()*Matrix3d::Identity();
// 			// Update deformation tensor
// 			Lp[p]->F = (Matrix3d::Identity() + 0.5*(L0+Lp[p]->L))*Lp[p]->F;
// 			// Lp[p]->F = (Matrix3d::Identity() + Lp[p]->L)*Lp[p]->F;
// 			// Update particle length
// 			CalPSizeR(p);
// 			// Update volume of particles
// 			Lp[p]->Vol 	= Lp[p]->F.determinant()*Lp[p]->Vol0;
// 		}
// 	}
// }

void MPM::AddNode(size_t level, Vector3d& x)
{
    Ln.push_back(new MPM_NODE(level,x));
    Ln[Ln.size()-1]->ID = Ln.size()-1;
}

void MPM::AddParticle(int tag, Vector3d& x, double m, double young, double poisson)
{
    Lp.push_back(new MPM_PARTICLE(tag,x,m, young, poisson));
    Lp[Lp.size()-1]->ID = Lp.size()-1;
}

void MPM::DeleteParticles()
{
	vector <MPM_PARTICLE*>	Lpt;
	Lpt.resize(0);

	for (size_t p=0; p<Lp.size(); ++p)
	{
		if (!Lp[p]->Removed)	Lpt.push_back(Lp[p]);
	}
	Lp = Lpt;

	for (size_t p=0; p<Lp.size(); ++p)
	{
		Lp[p]->ID = p;
	}
}

void MPM::AddBoxParticles(Vector3d& x0, Vector3d& l, double ratio, double m, double young, double poisson)
{
	Vector3i maxx = Vector3i::Zero();
	for (size_t d=0; d<D; ++d)
	{
		maxx(d) = (l(d)/ratio)-1;
	}

	for (int k=0; k<=maxx(2); ++k)
    for (int j=0; j<=maxx(1); ++j)
    for (int i=0; i<=maxx(0); ++i)
    {
    	int type = -1;

    	if (D==1)
    	{
    		if (i==0 || i==maxx(0))	type = -2;
    	}
    	else if (D==2)
    	{
    		if (i==0 || i==maxx(0) || j==0 || j==maxx(1))	type = -2;		
    	}
    	else if (D==3)
    	{
    		if (i==0 || i==maxx(0) || j==0 || j==maxx(1) || k==0 || k==maxx(2))		type = -2;
    	}

    	Vector3d x = Vector3d::Zero();
    					x(0) = ratio*(i + 0.5)+x0(0);
    	if (D>1)		x(1) = ratio*(j + 0.5)+x0(1);
    	if (D>2)		x(2) = ratio*(k + 0.5)+x0(2);

    	AddParticle(type, x, m, young, poisson);
    }

    Lbp.resize(0);
    for (size_t p=0; p<Lp.size(); ++p)
    {
    	Lp[p]->Vol0 = 1.;
    	for (size_t d=0; d<D; ++d)
    	{
    		Lp[p]->Vol0 *= ratio;
    		if (Ntype==3)	Lp[p]->PSize0(d) = 0.5*ratio;
    		else 			Lp[p]->PSize0(d) = 0.;
    		Lp[p]->PSize(d) = Lp[p]->PSize0(d);
    	}
    	Lp[p]->Vol 	= Lp[p]->Vol0;

    	if (Lp[p]->Type==-2)
    	{
    		Lbp.push_back(Lp[p]);
    	}
    }
}

// void DEM::LoadMPMFromH5( string fname)
// {
// 	cout << "========= Start loading MPM particles from " << fname << "==============" << endl;
// 	H5std_string FILE_NAME( fname );
// 	H5std_string DATASET_NAME_POS( "Position" );
// 	H5File file_pos( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_pos = file_pos.openDataSet( DATASET_NAME_POS );
// 	DataSpace dataspace_pos = dataset_pos.getSpace();
//     hsize_t dims_pos[2];
//     dataspace_pos.getSimpleExtentDims( dims_pos, NULL);
//     hsize_t dimsm_pos = dims_pos[0];

// 	H5std_string DATASET_NAME_VEL( "Velocity" );
// 	H5File file_vel( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_vel = file_vel.openDataSet( DATASET_NAME_VEL );
// 	DataSpace dataspace_vel = dataset_vel.getSpace();
//     hsize_t dims_vel[2];
//     dataspace_vel.getSimpleExtentDims( dims_vel, NULL);
//     hsize_t dimsm_vel = dims_vel[0];

// 	H5std_string DATASET_NAME_STR( "Stress" );
// 	H5File file_str( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_str = file_str.openDataSet( DATASET_NAME_STR );
// 	DataSpace dataspace_str = dataset_str.getSpace();
//     hsize_t dims_str[2];
//     dataspace_str.getSimpleExtentDims( dims_str, NULL);
//     hsize_t dimsm_str = dims_str[0];

// 	H5std_string DATASET_NAME_TAG( "Tag" );
// 	H5File file_tag( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_tag = file_tag.openDataSet( DATASET_NAME_TAG );
// 	DataSpace dataspace_tag = dataset_tag.getSpace();
//     hsize_t dims_tag[2];
//     dataspace_tag.getSimpleExtentDims( dims_tag, NULL);
//     hsize_t dimsm_tag = dims_tag[0];

// 	H5std_string DATASET_NAME_M( "Mass" );
// 	H5File file_m( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_m = file_m.openDataSet( DATASET_NAME_M );
// 	DataSpace dataspace_m = dataset_m.getSpace();
//     hsize_t dims_m[2];
//     dataspace_m.getSimpleExtentDims( dims_m, NULL);
//     hsize_t dimsm_m = dims_m[0];

// 	H5std_string DATASET_NAME_you( "Young" );
// 	H5File file_you( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_you = file_you.openDataSet( DATASET_NAME_you );
// 	DataSpace dataspace_you = dataset_you.getSpace();
//     hsize_t dims_you[2];
//     dataspace_you.getSimpleExtentDims( dims_you, NULL);
//     hsize_t dimsm_you = dims_you[0];

// 	H5std_string DATASET_NAME_poi( "Poisson" );
// 	H5File file_poi( FILE_NAME, H5F_ACC_RDONLY );
// 	DataSet dataset_poi = file_poi.openDataSet( DATASET_NAME_poi );
// 	DataSpace dataspace_poi = dataset_poi.getSpace();
//     hsize_t dims_poi[2];
//     dataspace_poi.getSimpleExtentDims( dims_poi, NULL);
//     hsize_t dimsm_poi = dims_poi[0];

//     double data_pos[dimsm_pos];
//     dataset_pos.read( data_pos, PredType::NATIVE_DOUBLE, dataspace_pos, dataspace_pos );

//     double data_vel[dimsm_vel];
//     dataset_vel.read( data_vel, PredType::NATIVE_DOUBLE, dataspace_vel, dataspace_vel );

//     double data_tag[dimsm_tag];
//     dataset_tag.read( data_tag, PredType::NATIVE_DOUBLE, dataspace_tag, dataspace_tag );

//     int np = dimsm_pos/3;
//     for (int i=6; i<np; ++i)
//     {
//     	Vector3d pos (scale*data_pos[3*i], scale*data_pos[3*i+1], scale*data_pos[3*i+2]);
//     	int tag = (int) data_tag[i];
//     	AddParticle(tag, pos, m, young, poisson);
//     }
//     cout << "========= Loaded "<< Lp.size()-6<< " DEM particles from " << fname << "==============" << endl;
// }

inline void MPM::WriteFileH5(int n)
{
	stringstream	out;							//convert int to string for file name.
	out << setw(6) << setfill('0') << n;			
	string file_name_h5 = "MPM_"+out.str()+".h5";

	H5File	file(file_name_h5, H5F_ACC_TRUNC);		//create a new hdf5 file.
	
	hsize_t	dims_scalar[1] = {Lp.size()};			//create data space.
	hsize_t	dims_vector[1] = {3*Lp.size()};			//create data space.
	hsize_t	dims_tensor[1] = {6*Lp.size()};

	int rank_scalar = sizeof(dims_scalar) / sizeof(hsize_t);
	int rank_vector = sizeof(dims_vector) / sizeof(hsize_t);
	int rank_tensor = sizeof(dims_tensor) / sizeof(hsize_t);

	DataSpace	*space_scalar = new DataSpace(rank_scalar, dims_scalar);
	DataSpace	*space_vector = new DataSpace(rank_vector, dims_vector);
	DataSpace	*space_tensor = new DataSpace(rank_tensor, dims_tensor);

	double* tag_h5 	= new double[  Lp.size()];
	double* m_h5 	= new double[  Lp.size()];
	double* you_h5 	= new double[  Lp.size()];
	double* poi_h5 	= new double[  Lp.size()];
	double* pos_h5 	= new double[3*Lp.size()];
	double* vel_h5 	= new double[3*Lp.size()];
	double* s_h5 	= new double[6*Lp.size()];

	for (size_t i=0; i<Lp.size(); ++i)
	{
        tag_h5[  i  ] 	= Lp[i]->Tag;
        m_h5  [  i  ] 	= Lp[i]->M;
        you_h5[  i  ] 	= Lp[i]->Young;
        poi_h5[  i  ] 	= Lp[i]->Poisson;
		pos_h5[3*i  ] 	= Lp[i]->X(0);
		pos_h5[3*i+1] 	= Lp[i]->X(1);
		pos_h5[3*i+2] 	= Lp[i]->X(2);
		vel_h5[3*i  ] 	= Lp[i]->V(0);
		vel_h5[3*i+1] 	= Lp[i]->V(1);
		vel_h5[3*i+2] 	= Lp[i]->V(2);
		s_h5  [6*i  ] 	= Lp[i]->S(0,0);
		s_h5  [6*i+1] 	= Lp[i]->S(0,1);
		s_h5  [6*i+2] 	= Lp[i]->S(0,2);
		s_h5  [6*i+3] 	= Lp[i]->S(1,1);
		s_h5  [6*i+4] 	= Lp[i]->S(1,2);
		s_h5  [6*i+5] 	= Lp[i]->S(2,2);
	}

	DataSet	*dataset_tag	= new DataSet(file.createDataSet("Tag", PredType::NATIVE_DOUBLE, *space_scalar));
	DataSet	*dataset_m		= new DataSet(file.createDataSet("Mass", PredType::NATIVE_DOUBLE, *space_scalar));
	DataSet	*dataset_you	= new DataSet(file.createDataSet("Young", PredType::NATIVE_DOUBLE, *space_scalar));
	DataSet	*dataset_poi	= new DataSet(file.createDataSet("Poisson", PredType::NATIVE_DOUBLE, *space_scalar));
    DataSet	*dataset_pos	= new DataSet(file.createDataSet("Position", PredType::NATIVE_DOUBLE, *space_vector));
    DataSet	*dataset_vel	= new DataSet(file.createDataSet("Velocity", PredType::NATIVE_DOUBLE, *space_vector));
    DataSet	*dataset_s		= new DataSet(file.createDataSet("Stress", PredType::NATIVE_DOUBLE, *space_tensor));

	dataset_tag->write(tag_h5, PredType::NATIVE_DOUBLE);
	dataset_m->write(m_h5, PredType::NATIVE_DOUBLE);
	dataset_you->write(you_h5, PredType::NATIVE_DOUBLE);
	dataset_poi->write(poi_h5, PredType::NATIVE_DOUBLE);
	dataset_pos->write(pos_h5, PredType::NATIVE_DOUBLE);
	dataset_vel->write(vel_h5, PredType::NATIVE_DOUBLE);
	dataset_s->write(s_h5, PredType::NATIVE_DOUBLE);

	delete space_scalar;
	delete space_vector;
	delete space_tensor;
	delete dataset_tag;
	delete dataset_m;
	delete dataset_you;
	delete dataset_poi;
	delete dataset_pos;
	delete dataset_vel;
	delete dataset_s;

	delete tag_h5;
	delete m_h5;
	delete you_h5;
	delete poi_h5;
	delete pos_h5;
	delete vel_h5;
	delete s_h5;

	file.close();

	string file_name_xmf = "MPM_"+out.str()+".xmf";

    std::ofstream oss;
    oss.open(file_name_xmf);
    oss << "<?xml version=\"1.0\" ?>\n";
    oss << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n";
    oss << "<Xdmf Version=\"2.0\">\n";
    oss << " <Domain>\n";
    oss << "   <Grid Name=\"MPM\" GridType=\"Uniform\">\n";
    oss << "     <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"" << Lp.size() << "\"/>\n";
    oss << "     <Geometry GeometryType=\"XYZ\">\n";
    oss << "       <DataItem Format=\"HDF\" NumberType=\"Float\" Precision=\"4\" Dimensions=\"" << Lp.size() << " 3\" >\n";
    oss << "        " << file_name_h5 <<":/Position \n";
    oss << "       </DataItem>\n";
    oss << "     </Geometry>\n";
    oss << "     <Attribute Name=\"Tag\" AttributeType=\"Scalar\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Lp.size() << "\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\">\n";
    oss << "        " << file_name_h5 <<":/Tag \n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "     <Attribute Name=\"Stress_ZZ\" AttributeType=\"Scalar\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Lp.size() << "\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\">\n";
    oss << "        " << file_name_h5 <<":/Stress_ZZ \n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "     <Attribute Name=\"Velocity\" AttributeType=\"Vector\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Lp.size() << " 3\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\">\n";
    oss << "        " << file_name_h5 <<":/Velocity\n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "     <Attribute Name=\"Stress\" AttributeType=\"Tensor6\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Lp.size() << " 6\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\">\n";
    oss << "        " << file_name_h5 <<":/Stress\n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "   </Grid>\n";
    oss << " </Domain>\n";
    oss << "</Xdmf>\n";
    oss.close();
}
