#include "Weapons/VRTProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "VRTCollision.h"
#include "VRTLog.h"

AVRTProjectile::AVRTProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(2.f);
	Collision->SetCollisionProfileName(VRTCollision::ProjectileProfile);
	Collision->SetNotifyRigidBodyCollision(true); // fire OnComponentHit
	Collision->SetCanEverAffectNavigation(false); // bullets fly through the navmesh area all the time
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.04f)); // engine sphere is 100 cm wide -> 4 cm bullet
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;

	InitialLifeSpan = Lifetime;
}

void AVRTProjectile::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentHit.AddDynamic(this, &AVRTProjectile::HandleHit);

	// The weapon and the player who fired must not stop the bullet at spawn (the muzzle can sit inside the capsule).
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Collision->IgnoreActorWhenMoving(GetInstigator(), true);
	VRT_LOG(LogVRTProjectile, Log, "Spawned at(World)=%s dir(World)=%s speed=%.0f cm/s", *GetActorLocation().ToCompactString(),
		*GetActorForwardVector().ToCompactString(), Movement->Velocity.Size());
}

void AVRTProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	VRT_LOG(LogVRTProjectile, Log, "Hit actor=%s component=%s at(World)=%s", *GetNameSafe(OtherActor), *GetNameSafe(OtherComp),
		*Hit.ImpactPoint.ToCompactString());

	// Debug hit effect: a red flash at the impact point.
	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, HitMarkerRadius, 8, FColor::Red, false, 1.f, 0, 0.5f);

	if (OtherActor && Damage > 0.f)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, Damage, GetVelocity().GetSafeNormal(), Hit, GetInstigatorController(), this, nullptr);
	}
	Destroy();
}
