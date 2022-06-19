#version 330 core
out vec4 FragColor;

// texture sampler
//uniform sampler2D texture1;

struct Material {
    sampler2D diffuse;
    vec3 specular;    
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

in vec3 FragPos;
in vec3 Normal;
in vec4 toColor;
in vec2 toTexCoord;

void main()
{
    vec3 ambient = light.ambient * texture(material.diffuse, toTexCoord).rgb;

     // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, toTexCoord).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);  

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0) * toColor;
}
