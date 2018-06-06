//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#pragma once

#include "BsCorePrerequisites.h"
#include "Image/BsColor.h"
#include "Image/BsColorGradient.h"
#include "Math/BsVector3.h"
#include "Animation/BsAnimationCurve.h"
#include "Math/BsRandom.h"
#include "Scene/BsSceneActor.h"
#include "CoreThread/BsCoreObject.h"
#include "Utility/BsModule.h"
#include "CoreThread/BsCoreThread.h"
#include "Image/BsPixelData.h"
#include "Math/BsAABox.h"
#include "Utility/BsBitwise.h"

namespace bs 
{
	class SkeletonMask;

	/** @addtogroup Particles-Internal
	 *  @{
	 */

	/** Determines type of distribution used by distribution properties. */
	enum PropertyDistributionType
	{
		/** The distribution is a costant value. */
		PDT_Constant,
		/** The distribution is a random value in a specified constant range. */
		PDT_RandomRange,
		/** The distribution is a time-varying value. */
		PDT_Curve,
		/** The distribution is a random value in a specified time-varying range. */
		PDT_RandomCurveRange
	};

	/* @} */

	/** @addtogroup Particles
	 *  @{
	 */

	/** Specifies a color as a distribution, which can include a constant color, random color range or a color gradient. */
	struct ColorDistribution
	{
		/** Creates a new distribution that returns a constant color. */
		ColorDistribution(const Color& color)
			: mType(PDT_Constant), mMinColor(color.getAsRGBA())
		{ }

		/** Creates a new distribution that returns a random color in the specified range. */
		ColorDistribution(const Color& minColor, const Color& maxColor)
			: mType(PDT_RandomRange), mMinColor(minColor.getAsRGBA()), mMaxColor(maxColor.getAsRGBA())
		{ }

		/** Creates a new distribution that evaluates a color gradient. */
		ColorDistribution(const ColorGradient& gradient)
			: mType(PDT_Curve), mMinGradient(gradient)
		{ }

		/** Creates a new distribution that returns a random color in a range determined by two gradients. */
		ColorDistribution(const ColorGradient& minGradient, const ColorGradient& maxGradient)
			: mType(PDT_RandomCurveRange), mMinGradient(minGradient), mMaxGradient(maxGradient)
		{ }

		/** 
		 * Evaluates the value of the distribution.
		 * 
		 * @param[in]	t		Time at which to evaluate the distribution. This is only relevant if the distribution
		 *						contains gradients.
		 * @param[in]	factor	Value in range [0, 1] that determines how to interpolate between min/max value, if the
		 *						distribution represents a range. Value of 0 will return the minimum value, while value of 1
		 *						will return the maximum value, and interpolate the values in-between.
		 * @return				Evaluated color.
		 *
		 */
		RGBA evaluate(float t, float factor) const
		{
			const UINT32 byteFactor = Bitwise::unormToUint<8>(factor);
			switch(mType)
			{
			default:
			case PDT_Constant:
				return mMinColor;
			case PDT_RandomRange:
				return Color::lerp(byteFactor, mMinColor, mMaxColor);
			case PDT_Curve:
				return mMinGradient.evaluate(t);
			case PDT_RandomCurveRange:
				{
					const RGBA minColor = mMinGradient.evaluate(t);
					const RGBA maxColor = mMaxGradient.evaluate(t);

					return Color::lerp(byteFactor, minColor, maxColor);
				}
			}
		}

	private:
		PropertyDistributionType mType;
		RGBA mMinColor;
		RGBA mMaxColor;
		ColorGradient mMinGradient;
		ColorGradient mMaxGradient;
	};

	/**  Specifies a floating point value as a distribution, which can include a constant value, random range or a curve. */
	struct FloatDistribution
	{
		/** Creates a new distribution that returns a constant value. */
		FloatDistribution(float value)
			: mType(PDT_Constant), mMinValue(value)
		{ }

		/** Creates a new distribution that returns a random value in the specified range. */
		FloatDistribution(float minValue, float maxValue)
			: mType(PDT_RandomRange), mMinValue(minValue), mMaxValue(maxValue)
		{ }

		/** Creates a new distribution that evaluates a curve. */
		FloatDistribution(const TAnimationCurve<float>& curve)
			: mType(PDT_Curve), mMinCurve(curve)
		{ }

		/** Creates a new distribution that returns a random value in a range determined by two curves. */
		FloatDistribution(const TAnimationCurve<float>& minCurve, const TAnimationCurve<float>& maxCurve)
			: mType(PDT_RandomCurveRange), mMinCurve(minCurve), mMaxCurve(maxCurve)
		{ }

		/** 
		 * Evaluates the value of the distribution.
		 * 
		 * @param[in]	t		Time at which to evaluate the distribution. This is only relevant if the distribution
		 *						contains curves.
		 * @param[in]	factor	Value in range [0, 1] that determines how to interpolate between min/max value, if the
		 *						distribution represents a range. Value of 0 will return the minimum value, while value of 1
		 *						will return the maximum value, and interpolate the values in-between.
		 * @return				Evaluated value.
		 *
		 */
		float evaluate(float t, float factor) const
		{
			switch(mType)
			{
			default:
			case PDT_Constant:
				return mMinValue;
			case PDT_RandomRange:
				return Math::lerp(factor, mMinValue, mMaxValue);
			case PDT_Curve:
				return mMinCurve.evaluate(t);
			case PDT_RandomCurveRange:
				{
					const float minValue = mMinCurve.evaluate(t);
					const float maxValue = mMaxCurve.evaluate(t);

					return Math::lerp(factor, minValue, maxValue);
				}
			}
		}
	private:
		PropertyDistributionType mType;
		float mMinValue;
		float mMaxValue;
		TAnimationCurve<float> mMinCurve;
		TAnimationCurve<float> mMaxCurve;
	};

	/** @} */

	/** @addtogroup Particles-Internal 
	 *  @{
	 */

	class ParticleSet;

	/** Contains particle emitter state that varies from frame to frame. */
	struct ParticleEmitterState
	{
		/** Emitter state relevant for skinned mesh emitter shape. */
		struct SkinnedMesh
		{
			Matrix4* bones;
			UINT32 numBones;
		};

		union
		{
			SkinnedMesh skinnedMesh;
		};
	};

	/** @} */

	/** @addtogroup Particles
	 *  @{
	 */

	/** 
	 * Base class from all emitter shapes. Emitter shapes determine the position and direction of newly created particles.
	 */
	class BS_CORE_EXPORT ParticleEmitterShape
	{
	public:
		virtual ~ParticleEmitterShape() = default;

		/** 
		 * Spawns a new set of particles using the current shape's distribution. 
		 *
		 * @param[in]	random		Random number generator.
		 * @param[in]	particles	Particle set in which to insert new particles.
		 * @param[in]	count		Number of particles to spawn.
		 * @param[in]	state		Optional state that can contain various per-frame information required for spawning
		 *							the particles.
		 * @return					Index at which the first of the particles was inserted, with other particles following
		 *							sequentially.
		 */
		virtual UINT32 spawn(const Random& random, ParticleSet& particles, UINT32 count, 
			const ParticleEmitterState& state) const = 0;

	protected:
		ParticleEmitterShape() = default;
	};


	/** Determines the emission type for the cone particle emitter shape. */
	enum class ParticleEmitterConeType
	{
		/** Emit particles only from the cone base. */
		Base,
		/** Emit particles from the entire cone volume. */
		Volume
	};

	/** Information describing a ParticleEmitterConeShape. */
	struct PARTICLE_CONE_SHAPE_DESC
	{
		/** Determines where on the cone are the particles emitter from. */
		ParticleEmitterConeType type = ParticleEmitterConeType::Base;

		/** Radius of the cone base. */
		float radius = 1.0f;

		/** Angle of the cone. */
		Degree angle = Degree(45.0f);

		/** Length of the cone. Irrelevant if emission type is Base. */
		float length = 1.0f;

		/** 
		 * Proportion of the volume that can emit particles. Thickness of 0 results in particles being emitted only from the
		 * edge of the cone, while thickness of 1 results in particles being emitted from the entire volume. In-between
		 * values will use a part of the volume.
		 */
		float thickness = 0.0f;

		/** Angular portion of the cone from which to emit particles from, in degrees. */
		Degree arc = Degree(360.0f);
	};

	/**
	 * Particle emitter shape that emits particles from a cone. Particles can be created on cone base or volume, while
	 * controling the radial arc of the emitted portion of the volume, as well as thickness of the cone emission volume.
	 * All particles will have random normals within the distribution of the cone.
	 */
	class BS_CORE_EXPORT ParticleEmitterConeShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterConeShape(const PARTICLE_CONE_SHAPE_DESC& desc);
		virtual ~ParticleEmitterConeShape() = default;

		/** Creates a new particle emitter cone shape. */
		static UPtr<ParticleEmitterConeShape> create(const PARTICLE_CONE_SHAPE_DESC& desc);
	protected:
		PARTICLE_CONE_SHAPE_DESC mInfo;
	};

	/** Information describing a ParticleEmitterSphereShape. */
	struct PARTICLE_SPHERE_SHAPE_DESC
	{
		/** Radius of the sphere. */
		float radius = 0.0f;

		/** 
		 * Proportion of the volume that can emit particles. Thickness of 0 results in particles being emitted only from the
		 * edge of the volume, while thickness of 1 results in particles being emitted from the entire volume. In-between
		 * values will use a part of the volume.
		 */
		float thickness = 0.0f;
	};

	/**
	 * Particle emitter shape that emits particles from a sphere. Particles can be emitted from sphere surface, the entire
	 * volume or a proportion of the volume depending on the thickness parameter. All particles will have normals pointing
	 * outwards in a spherical direction.
	 */
	class BS_CORE_EXPORT ParticleEmitterSphereShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterSphereShape(const PARTICLE_SPHERE_SHAPE_DESC& desc);

		/** @copydoc ParticleEmitterShape::spawn */
		UINT32 spawn(const Random& random, ParticleSet& particles, UINT32 count, 
			const ParticleEmitterState& state) const override;

		/** Spawns a single particle, generating its position and normal. */
		void spawn(const Random& random, Vector3& position, Vector3& normal) const;

		/** Creates a new particle emitter sphere shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_SPHERE_SHAPE_DESC& desc);
	protected:
		PARTICLE_SPHERE_SHAPE_DESC mInfo;
	};

	/** Information describing a ParticleEmitterHemisphereShape. */
	struct PARTICLE_HEMISPHERE_SHAPE_DESC
	{
		/** Radius of the hemisphere. */
		float radius = 0.0f;

		/** 
		 * Proportion of the volume that can emit particles. Thickness of 0 results in particles being emitted only from the
		 * edge of the volume, while thickness of 1 results in particles being emitted from the entire volume. In-between
		 * values will use a part of the volume.
		 */
		float thickness = 0.0f;
	};

	/**
	 * Particle emitter shape that emits particles from a hemisphere. Particles can be emitted from the hemisphere surface,
	 * the entire volume or a proportion of the volume depending on the thickness parameter. All particles will have 
	 * normals pointing outwards in a spherical direction.
	 */
	class BS_CORE_EXPORT ParticleEmitterHemisphereShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterHemisphereShape(const PARTICLE_HEMISPHERE_SHAPE_DESC& desc);

		/** @copydoc ParticleEmitterShape::spawn */
		UINT32 spawn(const Random& random, ParticleSet& particles, UINT32 count, 
			const ParticleEmitterState& state) const override;

		/** Spawns a single particle, generating its position and normal. */
		void spawn(const Random& random, Vector3& position, Vector3& normal) const;

		/** Creates a new particle emitter sphere shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_HEMISPHERE_SHAPE_DESC& desc);

	protected:
		PARTICLE_HEMISPHERE_SHAPE_DESC mInfo;
	};

	/** Determines the emission type for the cone particle emitter shape. */
	enum class ParticleEmitterBoxType
	{
		/** Particles will be emitted from the entire volume. */
		Volume,
		/** Particles will be emitted only from box surface. */
		Surface, 
		/** Particles will be emitted only from box edge. */
		Edge
	};

	/** Information describing a ParticleEmitterBoxShape. */
	struct PARTICLE_BOX_SHAPE_DESC
	{
		/** Determines from which portion of the box should particles be emitted from. */
		ParticleEmitterBoxType type = ParticleEmitterBoxType::Volume;

		/** Extends of the box. */
		Vector3 extents = Vector3::ONE;
	};

	/** 
	 * Particle emitter shape that emits particles from an axis aligned box. Particles can be emitted from box volume,
	 * surface or edges. All particles have their normals set to positive Z direction.
	 */
	class BS_CORE_EXPORT ParticleEmitterBoxShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterBoxShape(const PARTICLE_BOX_SHAPE_DESC& desc);
		virtual ~ParticleEmitterBoxShape() = default;

		/** Creates a new particle emitter box shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_BOX_SHAPE_DESC& desc);

	protected:
		PARTICLE_BOX_SHAPE_DESC mInfo;
	};

	/** Information describing a ParticleEmitterLineShape. */
	struct PARTICLE_LINE_SHAPE_DESC
	{
		/** Length of the line. */
		float length = 1.0f;
	};

	/** Particle emitter shape that emits particles from a line segment. */
	class BS_CORE_EXPORT ParticleEmitterLineShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterLineShape(const PARTICLE_LINE_SHAPE_DESC& desc);

		/** @copydoc ParticleEmitterShape::spawn */
		UINT32 spawn(const Random& random, ParticleSet& particles, UINT32 count, 
			const ParticleEmitterState& state) const override;

		/** Spawns a single particle, generating its position and normal. */
		void spawn(const Random& random, Vector3& position, Vector3& normal) const;

		/** Creates a new particle emitter edge shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_LINE_SHAPE_DESC& desc);

	protected:
		PARTICLE_LINE_SHAPE_DESC mInfo;
	};

	/** Information describing a ParticleEmitterCircleShape. */
	struct PARTICLE_CIRCLE_SHAPE_DESC
	{
		/** Radius of the circle. */
		float radius = 1.0f;

		/** 
		 * Proportion of the surface that can emit particles. Thickness of 0 results in particles being emitted only from
		 * the edge of the circle, while thickness of 1 results in particles being emitted from the entire surface. 
		 * In-between values will use a part of the surface.
		 */
		float thickness = 0.0f;

		/** Angular portion of the cone from which to emit particles from, in degrees. */
		Degree arc = Degree(360.0f);
	};

	/** 
	 * Particle emitter shape that emits particles from a circle. Using the thickness parameter you can control whether to
	 * emit only from circle edge, the entire surface or just a part of the surface. Using the arc parameter you can emit
	 * from a specific angular portion of the circle.
	 */
	class BS_CORE_EXPORT ParticleEmitterCircleShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterCircleShape(const PARTICLE_CIRCLE_SHAPE_DESC& desc);
		virtual ~ParticleEmitterCircleShape() = default;

		/** Creates a new particle emitter circle shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_CIRCLE_SHAPE_DESC& desc);

	protected:
		PARTICLE_CIRCLE_SHAPE_DESC mInfo;
	};

	/** Information describing a ParticleEmitterRectShape. */
	struct PARTICLE_RECT_SHAPE_DESC
	{
		/** Extents of the rectangle. */
		Vector2 extents = Vector2::ONE;
	};

	/** Particle emitter shape that emits particles from the surface of a rectangle. */
	class BS_CORE_EXPORT ParticleEmitterRectShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterRectShape(const PARTICLE_RECT_SHAPE_DESC& desc);

		/** @copydoc ParticleEmitterShape::spawn */
		UINT32 spawn(const Random& random, ParticleSet& particles, UINT32 count, 
			const ParticleEmitterState& state) const override;

		/** Spawns a single particle, generating its position and normal. */
		void spawn(const Random& random, Vector3& position, Vector3& normal) const;

		/** Creates a new particle emitter rectangle shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_RECT_SHAPE_DESC& desc);

	protected:
		PARTICLE_RECT_SHAPE_DESC mInfo;
	};

	/** Determines the emission type for the mesh particle emitter shape. */
	enum class ParticleEmitterMeshType
	{
		/** Particles will be emitted from mesh vertices. */
		Vertex, 
		/** Particles will be emitted from mesh edges. */
		Edge, 
		/** Particles will be emitted from mesh triangles. */
		Triangle
	};

	/** Information describing a ParticleEmitterStaticMeshShape and ParticleEmitterSkinnedMeshShape. */
	struct PARTICLE_MESH_SHAPE_DESC
	{
		/** Determines from which portion of the mesh are the particles emitted from. */
		ParticleEmitterMeshType type = ParticleEmitterMeshType::Triangle;

		/** 
		 * Data describing the mesh vertices and indices. Must not be null and must at least contain the following
		 * attributes:
		 *  - VES_POSITION of VET_FLOAT3 type, representing vertex positions. Required for both static and skinned emitters.
		 *  - VES_BLEND_INDICES of VET_UBYTE4 type, representing bone indices. Required only for skinned emitters.
		 *  - VES_BLEND_WEIGHTS of VET_FLOAT4 type, representing bone weights. Required only for skinned emitters.
		 *  - VES_NORMAL of either VET_FLOAT3 or VET_UBYTE4_NORM type, representing vertex normals. Optional for both
		 *    static and skinned emitters.
		 */
		SPtr<MeshData> meshData;
	};

	/** 
	 * Particle emitter shape that emits particles from a surface of a static (non-animated) mesh. Particles can be
	 * emitted from mesh vertices, edges or triangles. If information about normals exists, particles will also inherit
	 * the normals.
	 */
	class BS_CORE_EXPORT ParticleEmitterStaticMeshShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterStaticMeshShape(const PARTICLE_MESH_SHAPE_DESC& desc);
		virtual ~ParticleEmitterStaticMeshShape() = default;

		/** Creates a new particle emitter static mesh shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_MESH_SHAPE_DESC& desc);

	protected:
		PARTICLE_MESH_SHAPE_DESC mInfo;
		UINT8* mVertices;
		UINT8* mNormals;
		UINT32 mNumVertices;
		UINT32 mVertexStride;
		bool m32BitNormals;
	};

	/** Particle emitter shape that emits particles from a surface of a skinned (animated) mesh. Particles can be
	 * emitted from mesh vertices, edges or triangles. If information about normals exists, particles will also inherit
	 * the normals.
	 */
	class BS_CORE_EXPORT ParticleEmitterSkinnedMeshShape : public ParticleEmitterShape
	{
	public:
		ParticleEmitterSkinnedMeshShape(const PARTICLE_MESH_SHAPE_DESC& desc);
		virtual ~ParticleEmitterSkinnedMeshShape() = default;

		/** Creates a new particle emitter skinned mesh shape. */
		static UPtr<ParticleEmitterShape> create(const PARTICLE_MESH_SHAPE_DESC& desc);
	protected:
		/** Evaluates a blend matrix for a vertex at the specified index. */
		Matrix4 getBlendMatrix(const ParticleEmitterState& state, UINT32 vertexIdx) const;

		PARTICLE_MESH_SHAPE_DESC mInfo;
		UINT8* mVertices;
		UINT8* mNormals;
		UINT8* mBoneIndices;
		UINT8* mBoneWeights;
		UINT32 mNumVertices;
		UINT32 mVertexStride;
		bool m32BitNormals;
	};

	/** @} */

	/** @addtogroup Particles-Internal
	 *  @{
	 */

	class ParticleSystem;

	// TODO - Doc
	class BS_CORE_EXPORT ParticleModule
	{
	protected:
		ParticleModule() = default;
		virtual ~ParticleModule() = default;

		void setParent(ParticleSystem* parent) { mParent = parent; }

		ParticleSystem* mParent = nullptr;
	};

	// TODO - Doc
	struct ParticleRenderData
	{
		PixelData positionAndRotation;
		PixelData color;
		PixelData size;

		UINT32 numParticles;
		AABox bounds;
	};

	namespace ct { class ParticleSystem; }

	/** @} */

	/** @addtogroup Particles
	 *  @{
	 */

	// TODO - Doc
	enum class ParticleOrientation
	{
		ViewPlane,
		ViewPosition,
		Axis
	};

	// TODO - Doc
	class BS_CORE_EXPORT ParticleEmitter : public ParticleModule
	{
	public:
		ParticleEmitter()
			:mShape(bs_unique_ptr<ParticleEmitterShape>(nullptr))
		{
			
		}

		void setShape(UPtr<ParticleEmitterShape> shape) { mShape = std::move(shape); }
		ParticleEmitterShape* getShape() const { return mShape.get(); }

		void spawn(Random& random, const ParticleEmitterState& state, ParticleSet& set) const;

	private:
		UPtr<ParticleEmitterShape> mShape;
	};

	// TODO - Doc
	class BS_CORE_EXPORT ParticleEvolver : public ParticleModule
	{
	public:
		ParticleEvolver() = default;
		virtual ~ParticleEvolver() = default;

		virtual void evolve(Random& random, ParticleSet& set) const = 0;
	};

	// TODO - Debug only
	class BS_CORE_EXPORT ParticleDebugEvolver : public ParticleEvolver
	{
	public:
		void evolve(Random& random, ParticleSet& set) const;
	};

	// TODO - Doc
	class BS_CORE_EXPORT ParticleSystem final : public IReflectable, public CoreObject, public SceneActor, public INonCopyable
	{
	public:
		~ParticleSystem();

		void addEmitter(UPtr<ParticleEmitter> emitter)
		{
			mEmitters.push_back(std::move(emitter));
		}

		void addEvolver(UPtr<ParticleEvolver> evolver)
		{
			mEvolvers.push_back(std::move(evolver));
		}

		UINT32 getNumEmitters() const { return (UINT32)mEmitters.size(); }
		UINT32 getNumEvolvers() const { return (UINT32)mEvolvers.size(); }

		ParticleEmitter* getEmitter(UINT32 idx)
		{
			if(idx >= (UINT32)mEmitters.size())
				return nullptr;

			return mEmitters[idx].get();
		}

		ParticleEvolver* getEvolver(UINT32 idx)
		{
			if(idx >= (UINT32)mEvolvers.size())
				return nullptr;

			return mEvolvers[idx].get();
		}

		void removeEmitter(ParticleEmitter* emitter)
		{
			const auto iterFind = std::find_if(mEmitters.begin(), mEmitters.end(), 
				[emitter](const UPtr<ParticleEmitter>& curEmitter)
			{
				return curEmitter.get() == emitter;
				
			});

			if(iterFind != mEmitters.end())
				mEmitters.erase(iterFind);
		}

		void removeEvolver(ParticleEvolver* evolver)
		{
			const auto iterFind = std::find_if(mEvolvers.begin(), mEvolvers.end(), 
				[evolver](const UPtr<ParticleEvolver>& curEvolver)
			{
				return curEvolver.get() == evolver;
				
			});

			if(iterFind != mEvolvers.end())
				mEvolvers.erase(iterFind);
		}

		void setMaterial(const HMaterial& material)
		{
			mMaterial = material;
			_markCoreDirty();
		}

		const HMaterial& getMaterial() const { return mMaterial; }

		void simulate(float timeDelta, ParticleRenderData& output); // TODO - Mark as internal

		AABox calculateBounds() const;

		/**	Retrieves an implementation of the particle system usable only from the core thread. */
		SPtr<ct::ParticleSystem> getCore() const;

		/** Creates a new empty ParticleSystem object. */
		static SPtr<ParticleSystem> create();
	private:
		friend class ParticlesManager;

		friend class ParticleSystemRTTI;

		ParticleSystem();

		/** @copydoc CoreObject::createCore */
		SPtr<ct::CoreObject> createCore() const override;

		/** @copydoc SceneActor::_markCoreDirty */
		void _markCoreDirty(ActorDirtyFlag flag = ActorDirtyFlag::Everything) override;

		/** @copydoc CoreObject::syncToCore */
		CoreSyncData syncToCore(FrameAlloc* allocator) override;

		/**	Creates a new ParticleSystem instance without initializing it. */
		static SPtr<ParticleSystem> createEmpty();

		UINT32 mId = 0;

		Vector<UPtr<ParticleEmitter>> mEmitters;
		Vector<UPtr<ParticleEvolver>> mEvolvers;
		HMaterial mMaterial;

		Random mRandom;
		UPtr<ParticleSet> mParticleSet;

		/************************************************************************/
		/* 								RTTI		                     		*/
		/************************************************************************/
	public:
		friend class ParticleSystemRTTI;
		static RTTITypeBase* getRTTIStatic();
		RTTITypeBase* getRTTI() const override;
	};

	struct ParticleRenderDataGroup
	{
		UnorderedMap<UINT32, ParticleRenderData*> data;
	};

	/** @} */

	/** @addtogroup Particles-Internal
	 *  @{
	 */

	namespace ct
	{
		// TODO - Doc
		struct ParticleTextures
		{
			SPtr<Texture> positionAndRotation;
			SPtr<Texture> color;
			SPtr<Texture> size;
		};

		// TODO - Doc
		class BS_CORE_EXPORT ParticleSystem final : public CoreObject, public SceneActor
		{
		public:
			~ParticleSystem();

			/**	Sets an ID that can be used for uniquely identifying this object by the renderer. */
			void setRendererId(UINT32 id) { mRendererId = id; }

			/**	Retrieves an ID that can be used for uniquely identifying this object by the renderer. */
			UINT32 getRendererId() const { return mRendererId; }

			/** 
			 * Returns an ID that uniquely identifies the particle system. Can be used for locating evaluated particle 
			 * system render data in the structure output by the ParticlesManager. 
			 */
			UINT32 getId() const { return mId; }

			void setMaterial(const SPtr<Material>& material) { mMaterial = material; }
			const SPtr<Material>& getMaterial() const { return mMaterial; }


			/** @copydoc CoreObject::initialize */
			void initialize() override;
		private:
			friend class bs::ParticleSystem;

			ParticleSystem(UINT32 id)
				:mId(id)
			{ }

			/** @copydoc CoreObject::syncToCore */
			void syncToCore(const CoreSyncData& data) override;

			UINT32 mRendererId = 0;
			UINT32 mId;

			SPtr<Material> mMaterial;
		};
	}

	// TODO - Doc
	class BS_CORE_EXPORT ParticlesManager final : public Module<ParticlesManager>
	{
		struct Members;
	public:
		ParticlesManager();

		ParticleRenderDataGroup* update();

	private:
		friend class ParticleSystem;

		UINT32 registerParticleSystem(ParticleSystem* system);
		void unregisterParticleSystem(ParticleSystem* system);

		UPtr<Members> m;

		UINT32 mNextId = 1;
		UnorderedSet<ParticleSystem*> mSystems;

		bool mPaused = false;

		// Worker threads
		ParticleRenderDataGroup mRenderData[CoreThread::NUM_SYNC_BUFFERS];

		UINT32 mReadBufferIdx = 1;
		UINT32 mWriteBufferIdx = 0;
		
		Signal mWorkerDoneSignal;
		Mutex mMutex;

		UINT32 mNumActiveWorkers = 0;
		bool mSwapBuffers = false;
	};

	/** @} */
}
