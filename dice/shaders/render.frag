#version 330

// Varying変数
in vec2 texcoord;
in float f_faceID;

// ディスプレイへの出力変数
out vec4 out_color;

// 選択を判定するためのID
uniform int u_selectID;
uniform int u_selectFace;

//uniform変数
uniform sampler2D u_texture;

void main() {
    //選択有り
    if (u_selectID > 0) {
        // 選択のIDが0より大きければIDで描画する
		float c = f_faceID / 255.0;//
        out_color = vec4(c, c, c, 1.0);
    } else { //面選択無し
		if(u_selectID == 0){
			out_color = vec4(texture(u_texture, texcoord).rgb , 1.0);			
		}else{
				out_color = vec4(texture(u_texture, texcoord).rgb , 1.0);
			if(f_faceID ==u_selectFace ){
			}else{
				out_color = vec4(texture(u_texture, texcoord).rgb *0.3 , 1.0);
			}		
		}
	}
}
